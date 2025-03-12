import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/movement.wren" for PlayerMovement
import "gameplay/enemies/spawner.wren" for Spawner
import "gameplay/weapon.wren" for Pistol, Shotgun, Knife, Weapons
import "gameplay/camera.wren" for CameraVariables
import "gameplay/player.wren" for PlayerVariables

class Main {

    static Start(engine) {

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/NavmeshTest/LevelNavmeshTest.glb")

        // Loading sounds
        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")

        // Directional Light
        __directionalLight = engine.GetECS().NewEntity()
        __directionalLight.AddNameComponent().name = "Directional Light"

        var comp = __directionalLight.AddDirectionalLightComponent()
        comp.color = Vec3.new(4.0, 3.2, 1.2)
        comp.planes = Vec2.new(0.1, 1000.0)
        comp.orthographicSize = 75.0

        var transform = __directionalLight.AddTransformComponent()
        transform.translation = Vec3.new(-105.0, 68.0, 168.0)
        transform.rotation = Quat.new(-0.29, 0.06, -0.93, -0.19)

        // Player Setup

        __playerVariables = PlayerVariables.new()
        __playerMovement = PlayerMovement.new(false,0.0)
        __counter = 0
        __frameTimer = 0
        __groundedTimer = 0
        __hasDashed = false
        __timer = 0

        __enemyList = []
        __spawner = Spawner.new(Vec3.new(-41.8, 19.0, 269.6), 1000.0)

        __playerController = engine.GetECS().NewEntity()
        __camera = engine.GetECS().NewEntity()
        __player = engine.GetECS().NewEntity()

        var startPos = Vec3.new(25.0, 10.0, 50.0)

        __playerController.AddTransformComponent().translation = startPos
        __playerController.AddPlayerTag()
        __playerController.AddNameComponent().name = "PlayerController"
        __playerController.AddCheatsComponent().noClip = false

        var shape = ShapeFactory.MakeCapsuleShape(1.7, 0.5) // height, circle radius
        var rb = Rigidbody.new(engine.GetPhysics(), shape, true, false) // physics module, shape, isDynamic, allowRotation
        __playerController.AddRigidbodyComponent(rb)

        __cameraVariables = CameraVariables.new()

        var cameraProperties = __camera.AddCameraComponent()
        cameraProperties.fov = 45.0
        cameraProperties.nearPlane = 0.5
        cameraProperties.farPlane = 600.0
        cameraProperties.reversedZ = true

        __camera.AddTransformComponent()
        __camera.AddAudioEmitterComponent()
        __camera.AddNameComponent().name = "Camera"
        __camera.AddAudioListenerTag()

        __player.AddTransformComponent().translation = startPos
        __player.AddNameComponent().name = "Player"

        // Load Map
        engine.LoadModel("assets/models/blockoutv4.glb")

        // Gun Setup
        __gun = engine.LoadModel("assets/models/AnimatedRifle.glb")

        var gunTransform = __gun.GetTransformComponent()
        gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
        gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))

        var gunAnimations = __gun.GetAnimationControlComponent()
        gunAnimations.Play("Reload", 1.0, false)
        gunAnimations.Stop()

        __player.AttachChild(__camera)
        __camera.AttachChild(__gun)

        __armory = [Pistol.new(engine), Shotgun.new(engine), Knife.new(engine)]

        __activeWeapon = __armory[Weapons.pistol]
        __activeWeapon.equip(engine)

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)

        __ultimateCharge = 0
        __ultimateActive = false

        __enemyShape = ShapeFactory.MakeCapsuleShape(1.7, 0.5)
        __spawner.SpawnEnemies(engine, __enemyList, Vec3.new(0.01, 0.01, 0.01), 0.1, "assets/models/demon.glb", __enemyShape, 1)
    }

    static Shutdown(engine) {
        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {

        var cheats = __playerController.GetCheatsComponent()
        var deltaTime = engine.GetTime().GetDeltatime()
        __timer = __timer + dt

        __playerVariables.grenadeCharge = Math.Min(__playerVariables.grenadeCharge + __playerVariables.grenadeChargeRate * dt / 1000, __playerVariables.grenadeMaxCharge)

        if (__playerVariables.ultActive) {
            __playerVariables.ultCharge = Math.Max(__playerVariables.ultCharge - __playerVariables.ultDecayRate * dt / 1000, 0)
            if (__playerVariables.ultCharge <= 0) {
                __activeWeapon = __armory[Weapons.pistol]
                __activeWeapon.equip(engine)
                __playerVariables.ultActive = false
            }
        } else {
            __playerVariables.ultCharge = Math.Min(__playerVariables.ultCharge + __playerVariables.ultChargeRate * dt / 1000, __playerVariables.ultMaxCharge)
        }

        __playerVariables.grenadeCharge = Math.Min(__playerVariables.grenadeCharge + __playerVariables.grenadeChargeRate * dt / 1000, __playerVariables.grenadeMaxCharge)

        if(engine.GetInput().DebugGetKey(Keycode.eN())){
           cheats.noClip = !cheats.noClip
        }

        if (engine.GetInput().DebugIsInputEnabled()) {

            __playerMovement.Update(engine, dt, __playerController, __camera)

            for (weapon in __armory) {
                weapon.cooldown = Math.Max(weapon.cooldown - dt, 0)

                weapon.reloadTimer = Math.Max(weapon.reloadTimer - dt, 0)
                if (weapon != __activeWeapon) {
                    if (weapon.reloadTimer <= 0) {
                        weapon.ammo = weapon.maxAmmo
                    }
                }
            }

            if (engine.GetInput().GetDigitalAction("Reload").IsHeld()) {
                __activeWeapon.reload(engine)
            }

            if (engine.GetInput().GetDigitalAction("Shoot").IsHeld()) {
                __activeWeapon.attack(engine, dt, __cameraVariables)
            }

            // engine.GetInput().GetDigitalAction("Ultimate").IsPressed()
            if (engine.GetInput().DebugGetKey(Keycode.eU())) {
                if (__playerVariables.ultCharge == __playerVariables.ultMaxCharge) {
                    System.print("Activate ultimate")
                    __activeWeapon = __armory[Weapons.shotgun]
                    __activeWeapon.equip(engine)
                    __playerVariables.ultActive = true
                }
            }

            if (engine.GetInput().DebugGetKey(Keycode.eG())) {
                if (__playerVariables.grenadeCharge == __playerVariables.grenadeMaxCharge) {
                    // Throw grenade
                    __playerVariables.grenadeCharge = 0
                }
            }

            if (engine.GetInput().DebugGetKey(Keycode.eV())) {
                __armory[Weapons.knife].attack(engine, dt, __cameraVariables)
            }

            if (engine.GetInput().DebugGetKey(Keycode.e1())) {
                __activeWeapon = __armory[Weapons.pistol]
                __activeWeapon.equip(engine)
            }

            if (engine.GetInput().DebugGetKey(Keycode.e2())) {
                __activeWeapon = __armory[Weapons.shotgun]
                __activeWeapon.equip(engine)
            }

            __cameraVariables.Tilt(engine, __camera, deltaTime)
            __cameraVariables.Shake(engine, __camera, __timer)

            if (engine.GetInput().DebugGetKey(Keycode.eMINUS())) {
                __camera.GetCameraComponent().fov = __camera.GetCameraComponent().fov - Math.Radians(1)
            }
            if (engine.GetInput().DebugGetKey(Keycode.eEQUALS())) {
                __camera.GetCameraComponent().fov = __camera.GetCameraComponent().fov + Math.Radians(1)
            }

            if (engine.GetInput().DebugGetKey(Keycode.eLEFTBRACKET())) {
                __playerMovement.lookSensitivity = Math.Max(__playerMovement.lookSensitivity - 0.01, 0.01)
            }
            if (engine.GetInput().DebugGetKey(Keycode.eRIGHTBRACKET())) {
                __playerMovement.lookSensitivity = Math.Min(__playerMovement.lookSensitivity + 0.01, 10)
            }

            if (engine.GetInput().DebugGetKey(Keycode.eCOMMA())) {
                __playerVariables.DecreaseHealth(5)
            }
            if (engine.GetInput().DebugGetKey(Keycode.ePERIOD())) {
                __playerVariables.IncreaseHealth(5)
            }

            if (engine.GetInput().DebugGetKey(Keycode.eL())) {
                __playerVariables.IncreaseScore(1)
            }
        }

        engine.GetGame().GetHUD().UpdateHealthBar(__playerVariables.health / __playerVariables.maxHealth)
        engine.GetGame().GetHUD().UpdateAmmoText(__activeWeapon.ammo, __activeWeapon.maxAmmo)
        engine.GetGame().GetHUD().UpdateUltBar(__playerVariables.ultCharge / __playerVariables.ultMaxCharge)
        engine.GetGame().GetHUD().UpdateScoreText(__playerVariables.score)
        engine.GetGame().GetHUD().UpdateGrenadeBar(__playerVariables.grenadeCharge / __playerVariables.grenadeMaxCharge)

        var mousePosition = engine.GetInput().GetMousePosition()
        __playerMovement.lastMousePosition = mousePosition

        for (enemy in __enemyList) {
            enemy.Update(dt)
        }
    }
}