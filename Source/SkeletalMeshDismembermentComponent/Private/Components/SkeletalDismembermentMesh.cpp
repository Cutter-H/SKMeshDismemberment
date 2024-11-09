// CH (May 2024)


#include "Components/SkeletalDismembermentMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Net/UnrealNetwork.h"

void USkeletalDismembermentMesh::BeginPlay()
{
	Super::BeginPlay();
	SetIsReplicated(true);
	OnBoneDismemberDelegate.AddDynamic(this, &USkeletalDismembermentMesh::OnBoneBroken);
	if (!IsValid(ProxySkeletalMeshComponent))
		ProxySkeletalMeshComponent = this;

	if (IsValid(GetOwner()))
	{
		GetOwner()->OnTakePointDamage.AddDynamic(this, &USkeletalDismembermentMesh::OnPointDamage);
		GetOwner()->OnTakeRadialDamage.AddDynamic(this, &USkeletalDismembermentMesh::OnRadialDamage);
	}
	else
	{
		DestroyComponent();
	}
}

void USkeletalDismembermentMesh::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USkeletalDismembermentMesh, DismembermentBones);
}

TArray<FName> USkeletalDismembermentMesh::GetDismemberedBones() const
{
	return BrokenBones;
}

TArray<FBoneHealth> USkeletalDismembermentMesh::GetAllBonesHealth() const
{
	TArray<FBoneHealth> RetVal;
	for (FDismembermentRules boneRule : DismembermentBones)
	{
		RetVal.Add(FBoneHealth(boneRule.BoneName, boneRule.CumulativeBreakThreshold));
	}
	return RetVal;
}

float USkeletalDismembermentMesh::GetBoneHealth(FName boneName) const
{
	for (FDismembermentRules boneRule : DismembermentBones)
	{
		if(boneRule.BoneName == boneName)
			return boneRule.CumulativeBreakThreshold;
	}
	return -1.f;
}

void USkeletalDismembermentMesh::RebindPointDamage(FOnPointDamageTakenDel function){
	if (CurrentRadialDelegate.IsBound()){
		GetOwner()->OnTakeRadialDamage.Remove(CurrentRadialDelegate);
	}
	else{
		GetOwner()->OnTakePointDamage.RemoveDynamic(this, &USkeletalDismembermentMesh::OnPointDamage);
	}
	GetOwner()->OnTakePointDamage.AddUnique(function);
	CurrentPointDelegate = function;
}
void USkeletalDismembermentMesh::RebindRadialDamage(FOnRadialDamageTakenDel function) {
	if (CurrentRadialDelegate.IsBound()){
		GetOwner()->OnTakeRadialDamage.Remove(CurrentRadialDelegate);
	}
	else{
		GetOwner()->OnTakeRadialDamage.RemoveDynamic(this, &USkeletalDismembermentMesh::OnRadialDamage);
	}
	GetOwner()->OnTakeRadialDamage.AddUnique(function);
	CurrentRadialDelegate = function;
}

void USkeletalDismembermentMesh::SetProxySkeletalMeshComponent(USkeletalMeshComponent* proxyComponent)
{
	if (GetOwner()->GetLocalRole() == ROLE_Authority)
		ProxySkeletalMeshComponent = proxyComponent;
	else
		SetProxySkeletalMeshComponent_SERVER(proxyComponent);
}

void USkeletalDismembermentMesh::SetProxySkeletalMeshComponent_SERVER_Implementation(USkeletalMeshComponent* proxyComponent)
{
	ProxySkeletalMeshComponent = proxyComponent;
}

void USkeletalDismembermentMesh::OnPointDamage(AActor* DamagedActor, float Damage, AController* InstigatedBy, FVector HitLocation,
	UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const UDamageType* DamageType, AActor* DamageCauser)
{
	if ((FHitComponent != ProxySkeletalMeshComponent)) return;
	DamageBone(Damage, BoneName, DamageType, InstigatedBy, ShotFromDirection.GetSafeNormal() * DamageType->DamageImpulse, HitLocation);
}

void USkeletalDismembermentMesh::OnRadialDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, FVector Origin, 
	const FHitResult& HitInfo, AController* InstigatedBy, AActor* DamageCauser)
{

	float actorDistance = FVector::Distance(Origin, GetOwner()->GetActorLocation());
	for (FDismembermentRules boneRule : DismembermentBones)
	{
		if(!boneRule.bIgnoreRadialDamage && (actorDistance > FVector::Distance(GetSocketLocation(boneRule.BoneName), Origin)))
		{
			FVector impulse = (GetSocketLocation(boneRule.BoneName) - Origin).GetSafeNormal() * DamageType->DamageImpulse;
			DamageBone(Damage, boneRule.BoneName, DamageType, InstigatedBy, impulse, Origin);
		}
	}
}

bool USkeletalDismembermentMesh::DamageBone(float damage, FName boneName, const UDamageType* DamageType, AController* damageInstigator, FVector impulse, FVector breakHitLocation)
{
	bool bRetVal = false;
	for(int i=0;i<DismembermentBones.Num();i++)
	{
		if (DismembermentBones[i].IsBone(boneName) && (!(DismembermentBones[i].WhitelistedDamageTypes.Num() > 0) || DismembermentBones[i].WhitelistedDamageTypes.Contains(DamageType)))
		{
			bRetVal = true;
			OnBoneDamageReceived.Broadcast(boneName, damage, DismembermentBones[i].CumulativeBreakThreshold - damage, damageInstigator);
			bool bInstigatorAlreadyAdded = false;
			for (int j=0;j< DismembermentBones[i].DamageInstigators.Num();j++)
			{
				if (DismembermentBones[i].DamageInstigators[j].DamageInstigator == damageInstigator)
				{
					DismembermentBones[i].DamageInstigators[j].DamageDealt += damage;
					DismembermentBones[i].DamageInstigators[j].LastGameTimeOfDamageDealt = GetWorld()->TimeSeconds;
					bInstigatorAlreadyAdded = true;
				}
			}
			if (!bInstigatorAlreadyAdded)
			{
				DismembermentBones[i].DamageInstigators.Add(FDismembermentDamageInstigator(damageInstigator, damage, GetWorld()->TimeSeconds));
			}

			if (damage >= FMath::Min(DismembermentBones[i].CumulativeBreakThreshold, DismembermentBones[i].ImmediateBreakThreshold))
			{
				DismembermentBones[i].CumulativeBreakThreshold = 0;
				DismemberBone_MULTI(boneName, DismembermentBones[i].DamageInstigators, impulse, breakHitLocation, DismembermentBones[i].SimulateRule);
				
			}
			else
			{
				DismembermentBones[i].CumulativeBreakThreshold -= damage;
				if (DismembermentBones[i].bCalculateImmediateBreakThresholdInDuration && DismembermentBones[i].ImmediateBreakThresholdDuration > 0.f)
				{
					FTimerHandle timerHandle;
					FTimerDelegate timerDelegate;
					timerDelegate.BindUFunction(this, FName("ReturnBoneImmediateHealth"), damage, boneName, DamageType, timerHandle);

					GetWorld()->GetTimerManager().SetTimer(timerHandle, timerDelegate, DismembermentBones[i].ImmediateBreakThresholdDuration, false);
					if (timerHandle.IsValid())
						DismembermentBones[i].ImmediateBreakThreshold -= damage;
					
				}
			}
		}
	}
	return bRetVal;
}

void USkeletalDismembermentMesh::ReturnBoneImmediateHealth(float damage, FName boneName, const UDamageType* DamageType, FTimerHandle timerHandle)
{
	for (FDismembermentRules boneRule : DismembermentBones)
	{
		if (boneRule.IsBone(boneName) && (!(boneRule.WhitelistedDamageTypes.Num() > 0) || boneRule.WhitelistedDamageTypes.Contains(DamageType)))
		{
			boneRule.ImmediateBreakThreshold += damage;
		}
	}
	timerHandle.Invalidate();
}

void  USkeletalDismembermentMesh::DismemberBone_MULTI_Implementation(FName boneName, const TArray<FDismembermentDamageInstigator>& damageInstigators, FVector impulse, FVector breakHitLocation, ESimulateCondition simulateRules)
{
	BrokenBones.Add(boneName);
	int boneIndex = GetBoneIndex(boneName);
	switch (simulateRules)
	{
	case ESimulateCondition::SimulateSelfOnly:
		if (UPhysicsAsset* PhysAsset = GetPhysicsAsset())
		{
			for (int32 BodyIdx = 0; BodyIdx < Bodies.Num(); ++BodyIdx)
			{
				if (FBodyInstance* BodyInst = Bodies[BodyIdx])
				{
					if (UBodySetup* PhysAssetBodySetup = PhysAsset->SkeletalBodySetups[BodyIdx])
					{
						if (PhysAssetBodySetup->BoneName == boneName && PhysAssetBodySetup->PhysicsType == EPhysicsType::PhysType_Default)
						{
							BodyInst->SetInstanceSimulatePhysics(true);
						}
					}
				}
			}
		}
		break;
	case ESimulateCondition::SimulateAllLowerBones:
		SetAllBodiesBelowSimulatePhysics(boneName, true, true); 
		break;
	case ESimulateCondition::SimulateFullMesh:
		SetSimulatePhysics(true);
		break;
	}
	BreakConstraint(impulse, breakHitLocation, boneName);
	OnBoneBroken(boneName, GetSocketLocation(boneName), damageInstigators);
	OnBoneDismemberDelegate.Broadcast(boneName, GetSocketLocation(boneName), damageInstigators);
}

void USkeletalDismembermentMesh::ReplicateSimulateMesh_MULTI_Implementation()
{
	SetSimulatePhysics(true);
}
