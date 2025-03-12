import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/movement.wren" for PlayerMovement
import "gameplay/weapon.wren" for Pistol, Shotgun, Knife, Weapons
import "gameplay/camera.wren" for CameraVariables
import "gameplay/player.wren" for PlayerVariables

class Main {

    static Start(engine) {

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/NavmeshTest/LevelNavmeshTest.glb")
        engine.GetInput().SetMouseHidden(true)
        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")
        engine.GetGame().SetHUDEnabled(true)
          
        __playerVariables = PlayerVariables.new()
        __playerMovement = PlayerMovement.new(false,0.0)
        __counter = 0
        __frameTimer = 0
        __groundedTimer = 0
        __hasDashed = false
        __timer = 0

        __playerController = engine.GetECS().NewEntity()
        __camera = engine.GetECS().NewEntity()
        __player = engine.GetECS().NewEntity()

        __playerController.AddTransformComponent().translation = Vec3.new(-18.3, 30.3, 193.8)
        __playerController.AddPlayerTag()
        __playerController.AddNameComponent().name = "PlayerController"

        var shape = ShapeFactory.MakeCapsuleShape(1.7, 0.5) // height, circle radius
        var rb = Rigidbody.new(engine.GetPhysics(), shape, true, false) // physics module, shape, isDynamic, allowRotation
        __playerController.AddRigidbodyComponent(rb)

        __camera.AddTransformComponent()
        __camera.AddNameComponent().name = "Camera"
        __camera.AddAudioListenerTag()
        var cameraProperties = __camera.AddCameraComponent()
        cameraProperties.fov = 45.0
        cameraProperties.nearPlane = 0.5
        cameraProperties.farPlane = 600.0
        cameraProperties.reversedZ = true
        
        __cameraVariables = CameraVariables.new()
        
        __player.AddTransformComponent()
        __player.AddNameComponent().name = "Player"

        __player.AttachChild(__camera)

        __gun = engine.GetECS().GetEntityByName("AnimatedRifle")

        __camera.AttachChild(__gun)

        var gunAnimations = __gun.GetAnimationControlComponent()
        gunAnimations.Play("Reload", 1.0, false)
        gunAnimations.Stop()


        var clown = engine.GetECS().GetEntityByName("Clown")
        var clownAnimations = clown.GetAnimationControlComponent()
        clownAnimations.Play("NarutoRun", 1.0, true)
        clown.GetTransformComponent().translation = Vec3.new(7.5, 35.0, 285.0)
        clown.GetTransformComponent().scale = Vec3.new(0.01, 0.01, 0.01)

        if (__camera) {
            System.print("Player is online!")

            __camera.AddAudioEmitterComponent()
            __playerController.AddCheatsComponent()

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))
        }

        __armory = [Pistol.new(engine), Shotgun.new(engine), Knife.new(engine)]

        __activeWeapon = __armory[Weapons.pistol]
        __activeWeapon.equip(engine)

        // Inside cathedral: pentagram scene
        {   // Fire emitter 1
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-18.3, 30.3, 193.8), Vec3.new(0.0, 0.0, 0.0))
        }
        {   // Fire emitter 2
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-18.3, 30.3, 190.4), Vec3.new(0.0, 0.0, 0.0))
        }
        {   // Fire emitter 3
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-14.9, 30.3, 190.4), Vec3.new(0.0, 0.0, 0.0))
        }
        {   // Fire emitter 4
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-14.9, 30.3, 193.8), Vec3.new(0.0, 0.0, 0.0))
        }
        {   // Dust emitter
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eDust(), emitterFlags, Vec3.new(-17.0, 34.0, 196.0), Vec3.new(1.0, 0.0, 0.0))
        }

        // Attached mini island: pentagram scene (sprite sheet showcase)
        {   // Fire emitter 1
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFireAnimated(), emitterFlags, Vec3.new(30.5, 31.0, 168.0), Vec3.new(0.0, 0.0, 0.0))
        }

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)

        var enemyEntity = engine.LoadModel("assets/models/Demon.glb")[0]
        var enemyTransform = enemyEntity.GetTransformComponent()
        enemyTransform.scale = Vec3.new(0.03, 0.03, 0.03)
        enemyTransform.translation = Vec3.new(4.5, 35.0, 285.0)
    }

    static Shutdown(engine) {

        __camera.DetachChild(__gun)
        engine.GetECS().DestroyEntity(__playerController)
        engine.GetECS().DestroyEntity(__player)
        System.print("Shutdown script!")
    }

    static Update(engine, dt) {
        __counter = __counter + 1
        var cheats = __playerController.GetCheatsComponent()
        var deltaTime = engine.GetTime().GetDeltatime()
        __frameTimer = __frameTimer + dt
        __timer = __timer + dt

        if (__frameTimer > 1000.0) {
            __frameTimer = __frameTimer - 1000.0
            __counter = 0
        }

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

        var path = engine.GetPathfinding().FindPath(Vec3.new(-42.8, 19.3, 267.6), Vec3.new(-16.0, 29.0, 195.1))
    }
}