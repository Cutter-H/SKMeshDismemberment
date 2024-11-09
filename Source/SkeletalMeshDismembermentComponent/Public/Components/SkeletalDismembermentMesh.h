// CH (May 2024)

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/Event.h"
#include "SkeletalDismembermentMesh.generated.h"

DECLARE_DYNAMIC_DELEGATE_NineParams(FOnPointDamageTakenDel, AActor*, DamagedActor, float, Damage, AController*, InstigatedBy, FVector, HitLocation, UPrimitiveComponent*, HitComponent, FName, BoneName, FVector, ShotFromDirection, const UDamageType*, DamageType, AActor*, DamageCauser);
DECLARE_DYNAMIC_DELEGATE_SevenParams(FOnRadialDamageTakenDel, AActor*, DamagedActor, float, Damage, const UDamageType*, DamageType, FVector, Origin, const FHitResult&, HitInfo, AController*, InstigatedBy, AActor*, DamageCauser);

UENUM(BlueprintType)
enum ESimulateCondition {
	SimulateAllLowerBones, 
	SimulateSelfOnly,
	SimulateFullMesh
};
USTRUCT(BlueprintType)
struct FBoneHealth {
	GENERATED_BODY()
	/*
	* The bone's name.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Bones")
	FName BoneName;
	/*
	* Health of the bone.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Bones")
	float BoneHealth;
	FBoneHealth() {}
	FBoneHealth(FName boneName, float boneHealth)
	{
		BoneName = boneName;
		BoneHealth = boneHealth;
	}
};
/*
* Struct used for keeping track of damage from instigators.
*/
USTRUCT(BlueprintType)
struct FDismembermentDamageInstigator{
	GENERATED_BODY()
	/*
	* The actor that damaged the bone.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	AController* DamageInstigator;
	/* 
	* How much damage the actor has dealt.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	float DamageDealt = 0.f;
	/*
	* Game time at which the damage was last dealt from the instigator.
	* (Time in seconds since level began play, but IS paused when the game is paused, and IS dilated/clamped.)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Bones")
	float LastGameTimeOfDamageDealt = -1.f;
	/*
	* Default constructor.
	*/
	FDismembermentDamageInstigator() { DamageInstigator = NULL; }
	/*
	* Constructor that generates with an instigator.
	*/
	FDismembermentDamageInstigator(AController* damageInstigator, float damage)
	{
		DamageInstigator = damageInstigator;
		DamageDealt = damage;
	}
	/*
	* Constructor that generates with an instigator and time.
	*/
	FDismembermentDamageInstigator(AController* damageInstigator, float damage, float time)
	{
		DamageInstigator = damageInstigator;
		DamageDealt = damage;
		LastGameTimeOfDamageDealt = time;
	}
};

/*
* Struct used for rules used for dismembering bones.
*/
USTRUCT(BlueprintType)
struct FDismembermentRules
{
	GENERATED_BODY()

public:
	/*
	* The name of the bone that will follow these rules. 
	* If using a Humanoid skeleton it is recommended to not use the pelvis bone or similar on other hierarchies (root deformation bone).
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	FName BoneName = FName("None");
	/* 
	* If these bones are hit then it will register as this bone has been hit.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	TArray<FName> ProxyBones;
	/* 
	* How will the mesh simulate to this dismemberment.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	TEnumAsByte<ESimulateCondition> SimulateRule;
	/*
	* The amount of damage that can be dealt at once to break the bone.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	float ImmediateBreakThreshold = 20.f;
	/*
	* The amount of total damage before the bone breaks. 
	* Similar to a Health bar.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	float CumulativeBreakThreshold = 50.f;
	/*
	* If Damage dealt from multiple sources is calculated as the immediate break threshold during duration.
	* If this is false, ImmediateBreakThreshold can only determine single damage events.
	* (If true, this will generate FTimerHandles on damage dealt to this bone)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	bool bCalculateImmediateBreakThresholdInDuration = true;
	/*
	* Duration that will be calculated to reach ImmediateBreakThreshold. 
	* (bCalculateImmediateBreakThresholdInDuration must be true)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment", 
		meta = (EditCondition = "bCalculateImmediateBreakThresholdInDuration", EditConditionHides))
	float ImmediateBreakThresholdDuration = 0.f;
	/*
	* If true radial damage will be ignored and only Point damage will affect this bone.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	bool bIgnoreRadialDamage = false;
	/*
	* // TODO
	* Break descendent bones when this bone breaks.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	bool bBreakDescendentsOnSelfBreak = false;
	/*
	* Disable breaking on descendent bones after this bone is broken.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	bool bDisableBreakingOnDescendentsOnSelfBreak = false;
	/*
	* If any entries are added to this array, only these damage types affect this bone's dismemberment.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	TArray<TObjectPtr<UDamageType>> WhitelistedDamageTypes;
	/*
	* Damage types in this array do not affect this bone's dismemberment.
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Variables|Dismemberment")
	TArray<TObjectPtr<UDamageType>> BlacklistedDamageTypes;
	/*
	* List of instigators that have damaged this bone.
	*/
	TArray<FDismembermentDamageInstigator> DamageInstigators;
	/*
	* Default constructor.
	*/
	FDismembermentRules(){}

	bool IsBone(FName testingBoneName)
	{
		return (BoneName == testingBoneName || ProxyBones.Contains(testingBoneName));
	}

};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBoneDismemberDelegate, FName, boneName, FVector, disconnectionWorldLocation, const TArray<FDismembermentDamageInstigator>&, damagingActors);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBoneDamageReceived, FName, boneName, float, damageReceived, float, newHealth, AController*, instigator);


/**
 * A skeletal mesh component whose bones can take damage and be dismembered.
 */
UCLASS(ClassGroup = (Dismemberment), meta = (BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class SKELETALMESHDISMEMBERMENTCOMPONENT_API USkeletalDismembermentMesh : public USkeletalMeshComponent
{
	GENERATED_BODY()
	
public:
	/*
	* Bind to add functionality for when a bone is broken.
	*/
	UPROPERTY(BlueprintAssignable)
	FOnBoneDismemberDelegate OnBoneDismemberDelegate;
	/*
	* Bind to add functionality for when a bone is damaged.
	*/
	UPROPERTY(BlueprintAssignable)
	FOnBoneDamageReceived OnBoneDamageReceived;
	/*
	* Called when the component is spawned.
	*/
	virtual void BeginPlay() override;
	/*
	* Replicate vars.
	*/
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	/*
	* Override in Blueprints to add functionality for when a bone is broken.
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Dismemberment")
	void OnBoneBroken(FName boneName, FVector disconnectionWorldLocation, const TArray<FDismembermentDamageInstigator>& DamagingActors);
	/*
	* Returns an array of names of bones that have been dismembered.
	*/
	UFUNCTION(BlueprintCallable, Category = "Dismemberment")
	TArray<FName> GetDismemberedBones() const;
	/*
	* Get Health of all bones.
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Dismemberment")
	TArray<FBoneHealth> GetAllBonesHealth() const;
	/*
	* Get Health bone by name.
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Dismemberment")
	float GetBoneHealth(FName boneName) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Dismemberment")
	void RebindPointDamage(FOnPointDamageTakenDel function);
	FScriptDelegate CurrentPointDelegate;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Dismemberment")
	void RebindRadialDamage(FOnRadialDamageTakenDel function);
	FScriptDelegate CurrentRadialDelegate;
protected:
	/*
	* List of bones that can be dismembered and the rules for dismemberment.
	*/
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Dismemberment", meta = (TitleProperty = "BoneName"))
	TArray<FDismembermentRules> DismembermentBones;
	/*
	* Sets the component used for OnPointDamageTaken.
	*/
	UFUNCTION(BlueprintCallable, Category = "Dismemberment")
	void SetProxySkeletalMeshComponent(USkeletalMeshComponent* proxyComponent);

private:
	/*
	* (C++ Only) 
	* Component used for OnPointDamageTaken. 
	*/
	UPROPERTY(EditAnywhere, Category = "Dismemberment")
	TObjectPtr<USkeletalMeshComponent> ProxySkeletalMeshComponent;
	/*
	* (C++ Only) 
	* Array of names of broken bones.
	*/
	TArray<FName> BrokenBones;
	/*
	* (C++ Only) 
	* Server function to set the proxy skeletal mesh component, if the protect function is called from a client.
	*/
	UFUNCTION(Server, Reliable)
	void SetProxySkeletalMeshComponent_SERVER(USkeletalMeshComponent* proxyComponent);
	/*
	* (C++ Only) 
	* Function that is bound to the owning actor's OnTakePointDamage delegate.
	*/
	UFUNCTION()
	void OnPointDamage(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection, const class UDamageType* DamageType, AActor* DamageCauser);
	/*
	* (C++ Only) 
	* Function that is bound to the owning actor's OnTakeRadialDamage delegate.
	*/
	UFUNCTION()
	void OnRadialDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, FVector Origin, const FHitResult& HitInfo, class AController* InstigatedBy, AActor* DamageCauser);
	/*
	* (C++ Only) 
	* Primary function. Reduces the bones health bar.
	*/
	UFUNCTION()
	bool DamageBone(float damage, FName boneName, const UDamageType* DamageType, AController* damageInstigator, FVector impulse, FVector breakHitLocation);
	/*
	* (C++ Only) 
	* Timed function that restores damage dealt to the bone as immediate damage.
	*/
	UFUNCTION()
	void ReturnBoneImmediateHealth(float damage, FName boneName, const UDamageType* DamageType, FTimerHandle timerHandle);
	/*
	* (C++ Only) 
	* Dismembers the named bone.
	*/
	UFUNCTION(NetMulticast, Reliable)
	void DismemberBone_MULTI(FName boneName, const TArray<FDismembermentDamageInstigator>& damageInstigators, FVector impulse, FVector breakHitLocation, ESimulateCondition simulateRules);
	/*
	* Simulates the whole mesh.
	*/
	UFUNCTION(NetMulticast, Reliable)
	void ReplicateSimulateMesh_MULTI();
	/*
	*/



};
