# SkeletalMeshDismembermentComponent
 Unreal Engine plugin
 Skeletal Mesh Component that breaks when damage is dealt to it. 
 
 * You can make bones simulate only themselves, themself + all of their children bones, or the entire character.
 * Bones have their own individual health.
 * Dispatchers are called upon bone damage and break, so if you want to 'kill' the character upon destruction of the head then it is possible!
 * Supports both point and radial damage.

Drawback: This is a skeletal mesh component. So with the stock Character class of the engine you will need an additional skeletal mesh to be added.
To give further support, you can assign a proxy mesh. When the proxy mesh's hand_r bone is damaged, this skeletal mesh is damaged on the hand_r bone.

---
Demonstration video:

https://github.com/user-attachments/assets/eb679673-c76e-48f1-91ee-f84bdbad2721

