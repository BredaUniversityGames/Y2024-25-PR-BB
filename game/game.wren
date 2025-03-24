import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/movement.wren" for PlayerMovement
import "gameplay/weapon.wren" for Pistol, Shotgun, Knife, Weapons
import "gameplay/camera.wren" for CameraVariables
import "gameplay/player.wren" for PlayerVariables
import "analytics/analytics.wren" for AnalyticsManager

class Main {

    static Start(engine) {


        engine.GetGame().SetHUDEnabled(true)
        
        
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
        comp.planes = Vec2.new(-50.0, 500.0)
        comp.orthographicSize = 120.0

        var transform = __directionalLight.AddTransformComponent()
        transform.translation = Vec3.new(-94.000, 174.800, 156.900)
        transform.rotation = Quat.new(0.544, -0.136, -0.800,-0.214)

        // Player Setup

        __playerVariables = PlayerVariables.new()
        __playerMovement = PlayerMovement.new(false,0.0)
        __counter = 0
        __frameTimer = 0
        __groundedTimer = 0
        __hasDashed = false
        __timer = 0

        // Player stuff
        engine.GetInput().SetMouseHidden(true)
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

        var positions = [Vec3.new(0.0, 12.4, 11.4), Vec3.new(13.4, -0.6, 73.7), Vec3.new(24.9, -0.6, 72.3), Vec3.new(-30, 7.8, -10.2), Vec3.new(-41, 6.9, 1.2), Vec3.new(42.1, 12.4, -56.9)]

        __demons = []
        for (position in positions) {
            __demons.add(engine.LoadModel("assets/models/Demon.glb"))
            var demonAnimations = __demons[-1].GetAnimationControlComponent()
            demonAnimations.Play("Idle", 1.0, true, 0.0, false)
            __demons[-1].GetTransformComponent().translation = position
            __demons[-1].GetTransformComponent().scale = Vec3.new(0.025, 0.025, 0.025)
        }


        // Load Map
        engine.LoadModel("assets/models/blockoutv5.glb")

        // Loading lights from gltf, uncomment to test
        // engine.LoadModel("assets/models/light_test.glb")

        // Gun Setup
        __gun = engine.LoadModel("assets/models/AnimatedRifle.glb")

        var gunTransform = __gun.GetTransformComponent()
        gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
        gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))

        var gunAnimations = __gun.GetAnimationControlComponent()
        gunAnimations.Play("Reload", 1.0, false, 0.0, false)
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

        AnalyticsManager.AccuracyEvent(1, "Revolver", 1)
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

            if(engine.GetInput().DebugGetKey(Keycode.eK())) {
                for(demon in __demons) {
                    var demonAnimations = demon.GetAnimationControlComponent()
                    demonAnimations.Play("Walk", 1.0, true, 0.3, true)
                }
            }
            if(engine.GetInput().DebugGetKey(Keycode.eL())) {
                for(demon in __demons) {
                    var demonAnimations = demon.GetAnimationControlComponent()
                    demonAnimations.Play("Run", 2.0, true, 0.3, true)
                }
            }
            if(engine.GetInput().DebugGetKey(Keycode.eJ())) {
                for(demon in __demons) {
                    var demonAnimations = demon.GetAnimationControlComponent()
                    demonAnimations.Play("Attack", 1.0, false, 0.3, false)
                }
            }

            for(demon in __demons) {
                var demonAnimations = demon.GetAnimationControlComponent()
                if(demonAnimations.AnimationFinished()) {
                    demonAnimations.Play("Idle", 1.0, true, 0.0, false)
                }
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
        engine.GetGame().GetHUD().UpdateDashCharges(__playerMovement.currentDashCount)

        var mousePosition = engine.GetInput().GetMousePosition()
        __playerMovement.lastMousePosition = mousePosition

        // var path = engine.GetPathfinding().FindPath(Vec3.new(-42.8, 19.3, 267.6), Vec3.new(-16.0, 29.0, 195.1))
    }
}