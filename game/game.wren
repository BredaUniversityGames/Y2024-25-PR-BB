import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/movement.wren" for PlayerMovement
import "gameplay/enemies/spawner.wren" for Spawner
import "gameplay/weapon.wren" for Pistol, Shotgun, Knife, Weapons
import "gameplay/camera.wren" for CameraVariables
import "gameplay/player.wren" for PlayerVariables
import "gameplay/music_player.wren" for MusicPlayer, BGMPlayer
import "gameplay/wave_system.wren" for WaveSystem, WaveConfig, SpawnLocationType
import "analytics/analytics.wren" for AnalyticsManager

class Main {

    static Start(engine) {
        engine.GetInput().SetActiveActionSet("Shooter")
        engine.GetGame().SetHUDEnabled(true)

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/blockoutv5navmesh_04.glb")

        // Loading sounds
        engine.GetAudio().LoadBank("assets/music/Master.bank")
        engine.GetAudio().LoadBank("assets/music/Master.strings.bank")
        engine.GetAudio().LoadSFX("assets/sounds/slide2.wav", true, true)
        engine.GetAudio().LoadSFX("assets/sounds/crows.wav", true, false)

        engine.GetAudio().LoadSFX("assets/sounds/hit1.wav", false, false)
        engine.GetAudio().LoadSFX("assets/sounds/demon_roar.wav", true, false)

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
        __playerVariables.cameraVariables = __cameraVariables
        var cameraProperties = __camera.AddCameraComponent()
        cameraProperties.fov = Math.Radians(45.0)
        cameraProperties.nearPlane = 0.1
        cameraProperties.farPlane = 600.0
        cameraProperties.reversedZ = true

        __camera.AddTransformComponent()
        __camera.AddAudioEmitterComponent()
        __camera.AddNameComponent().name = "Camera"
        __camera.AddAudioListenerTag()

        __player.AddTransformComponent().translation = startPos
        __player.AddNameComponent().name = "Player"

        // Load Map
        engine.LoadModel("assets/models/blockoutv5_1.glb")

        engine.PreloadModel("assets/models/Skeleton.glb")

        engine.PreloadModel("assets/models/Revolver.glb")
        engine.PreloadModel("assets/models/Shotgun.glb")

        // Loading lights from gltf, uncomment to test
        // engine.LoadModel("assets/models/light_test.glb")

        // Gun Setup
        __gun = engine.LoadModel("assets/models/revolver.glb")
        __gun.RenderInForeground()

        __gun.GetNameComponent().name = "Gun"

        var gunTransform = __gun.GetTransformComponent()
        gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI()/2, 0.0))


        __player.AttachChild(__camera)
        __camera.AttachChild(__gun)

        __armory = [Pistol.new(engine), Shotgun.new(engine), Knife.new(engine)]

        __activeWeapon = __armory[Weapons.pistol]
        __activeWeapon.equip(engine)
        __nextWeapon = null
        // create the player movement
        __playerMovement = PlayerMovement.new(false,0.0,__activeWeapon)

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)

        __ultimateCharge = 0
        __ultimateActive = false

        __pauseEnabled = false

        // Music

        var ambientList = [
            "assets/music/ambient/207841__speedenza__dark-swamp-theme-1.wav",
            ""
            ]

        __musicPlayer = BGMPlayer.new(engine.GetAudio(),
            "event:/Gameplay",
            0.15)

        __ambientPlayer = MusicPlayer.new(engine.GetAudio(), ambientList, 0.1)

        var spawnLocations = []
        for(i in 0..7) {
            spawnLocations.add(engine.GetECS().GetEntityByName("Spawner_%(i)"))
        }

        __enemyList = []
        var waveConfigs = []
        waveConfigs.add(WaveConfig.new().SetDuration(10)
            .AddSpawn("Skeleton", SpawnLocationType.Closest, 1, 1)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 7, 3)
        )
        waveConfigs.add(WaveConfig.new().SetDuration(30)
            .AddSpawn("Skeleton", 0, 1, 1)
            .AddSpawn("Skeleton", 1, 1, 2)
            .AddSpawn("Skeleton", 2, 1, 1)
            .AddSpawn("Skeleton", 3, 1, 2)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 7, 3)
            .AddSpawn("Skeleton", 0, 10, 1)
            .AddSpawn("Skeleton", 1, 15, 1)
            .AddSpawn("Skeleton", 2, 5, 1)
            .AddSpawn("Skeleton", 3, 15, 3)
        )
        waveConfigs.add(WaveConfig.new().SetDuration(60)
            .AddSpawn("Skeleton", 0, 1, 2)
            .AddSpawn("Skeleton", 1, 1, 2)
            .AddSpawn("Skeleton", 2, 1, 1)
            .AddSpawn("Skeleton", 3, 1, 2)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 5, 5)
            .AddSpawn("Skeleton", 0, 15, 2)
            .AddSpawn("Skeleton", 1, 15, 1)
            .AddSpawn("Skeleton", 2, 15, 2)
            .AddSpawn("Skeleton", 3, 15, 3)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 15, 5)
            .AddSpawn("Skeleton", 0, 40, 3)
            .AddSpawn("Skeleton", 1, 40, 1)
            .AddSpawn("Skeleton", 2, 40, 2)
            .AddSpawn("Skeleton", 3, 40, 2)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 7, 5)
        )
        __waveSystem = WaveSystem.new(engine, waveConfigs, __enemyList, spawnLocations, __player)

        // Pause Menu callbacks

        __pauseHandler = Fn.new {
            __pauseEnabled = true
            engine.GetTime().SetScale(0.0)
            engine.GetGame().SetPauseMenuEnabled(true)
            engine.GetInput().SetActiveActionSet("UserInterface")
            engine.GetInput().SetMouseHidden(false)
            engine.GetUI().SetSelectedElement(engine.GetGame().GetPauseMenu().continueButton)
            __musicPlayer.SetVolume(engine.GetAudio(), 0.05)
            System.print("Pause Menu is %(__pauseEnabled)!")
        }

        __unpauseHandler = Fn.new {
            __pauseEnabled = false
            engine.GetTime().SetScale(1.0)
            engine.GetGame().SetPauseMenuEnabled(false)
            engine.GetInput().SetActiveActionSet("Shooter")
            engine.GetInput().SetMouseHidden(true)
            __musicPlayer.SetVolume(engine.GetAudio(), 0.15)
            System.print("Pause Menu is %(__pauseEnabled)!")
        }

        var continueButton = engine.GetGame().GetPauseMenu().continueButton
        continueButton.OnPress(__unpauseHandler)

        var backToMain = Fn.new {
            engine.TransitionToScript("game/main_menu.wren")
            engine.GetGame().SetPauseMenuEnabled(false)
            engine.GetGame().SetHUDEnabled(false)
            engine.GetTime().SetScale(1.0)
        }

        var menuButton = engine.GetGame().GetPauseMenu().backButton
        menuButton.OnPress(backToMain)
    }

    static Shutdown(engine) {
        engine.ResetDecals()

        __musicPlayer.Destroy(engine.GetAudio())
        __ambientPlayer.Destroy(engine.GetAudio())

        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {

        if (__enemyList.count != 0) {
            __musicPlayer.SetAttribute(engine.GetAudio(), "Intensity", 1.0)
        } else {
            __musicPlayer.SetAttribute(engine.GetAudio(), "Intensity", 0.0)
        }

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
                __playerVariables.wasUltReadyLastFrame = false
            }
        }

        if (!__playerVariables.wasUltReadyLastFrame && __playerVariables.ultCharge == __playerVariables.ultMaxCharge) {
            engine.GetAudio().PlayEventOnce("event:/UltReady")
            __playerVariables.wasUltReadyLastFrame = true
        }

        __playerVariables.invincibilityTime = Math.Max(__playerVariables.invincibilityTime - dt, 0)

        __playerVariables.multiplierTimer = Math.Max(__playerVariables.multiplierTimer - dt, 0)

        if (__playerVariables.multiplierTimer == 0 ) {
            __playerVariables.multiplier = 1.0
            __playerVariables.consecutiveHits = 0
        }


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

            // engine.GetInput().GetDigitalAction("Ultimate").IsPressed()
            if (engine.GetInput().DebugGetKey(Keycode.eU())) {
                if (__playerVariables.ultCharge >= __playerVariables.ultMaxCharge) {
                    System.print("Activate ultimate")
                    __activeWeapon = __armory[Weapons.shotgun]
                    __activeWeapon.equip(engine)
                    __playerVariables.ultActive = true

                    engine.GetAudio().PlayEventOnce("event:/ActivateUlt")

                    var particleEntity = engine.GetECS().NewEntity()
                    particleEntity.AddTransformComponent().translation = __player.GetTransformComponent().translation - Vec3.new(0,3.5,0)
                    var lifetime = particleEntity.AddLifetimeComponent()
                    lifetime.lifetime = 400.0
                    var emitterFlags = SpawnEmitterFlagBits.eIsActive()
                    engine.GetParticles().SpawnEmitter(particleEntity, EmitterPresetID.eHealth(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
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

            if (engine.GetInput().DebugGetKey(Keycode.e1()) && __activeWeapon.isUnequiping(engine) == false) {
                __activeWeapon.unequip(engine)
                __nextWeapon = __armory[Weapons.pistol]
            }

            if (engine.GetInput().DebugGetKey(Keycode.e2()) && __activeWeapon.isUnequiping(engine) == false) {
                __activeWeapon.unequip(engine)
                __nextWeapon = __armory[Weapons.shotgun]
            }

            if(__activeWeapon.isUnequiping(engine) == false && __nextWeapon != null){

                __activeWeapon = __nextWeapon
                __nextWeapon = null
                __activeWeapon.equip(engine)

            }
            if (engine.GetInput().GetDigitalAction("Reload").IsHeld() && __activeWeapon.isUnequiping(engine) == false) {
                __activeWeapon.reload(engine)
            }

            if (engine.GetInput().GetDigitalAction("Shoot").IsHeld()  && __activeWeapon.isUnequiping(engine) == false ) {
                __activeWeapon.attack(engine, dt, __playerVariables, __enemyList)
                if (__activeWeapon.ammo <= 0) {
                    __activeWeapon.reload(engine)
                }
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
                __spawnerList[0].SpawnEnemies(engine, __enemyList, Vec3.new(0.02, 0.02, 0.02), 5, "assets/models/Skeleton.glb", __enemyShape, 1)
            }
        }

        // Check if pause key was pressed
        if(engine.GetInput().GetDigitalAction("Menu").IsPressed()) {

            __pauseEnabled = !__pauseEnabled

            if (__pauseEnabled) {
                __pauseHandler.call()
            } else {
                __unpauseHandler.call()
            }
        }

        engine.GetGame().GetHUD().UpdateHealthBar(__playerVariables.health / __playerVariables.maxHealth)
        engine.GetGame().GetHUD().UpdateAmmoText(__activeWeapon.ammo, __activeWeapon.maxAmmo)
        engine.GetGame().GetHUD().UpdateUltBar(__playerVariables.ultCharge / __playerVariables.ultMaxCharge)
        engine.GetGame().GetHUD().UpdateScoreText(__playerVariables.score)
        engine.GetGame().GetHUD().UpdateGrenadeBar(__playerVariables.grenadeCharge / __playerVariables.grenadeMaxCharge)
        engine.GetGame().GetHUD().UpdateDashCharges(__playerMovement.currentDashCount)
        engine.GetGame().GetHUD().UpdateMultiplierText(__playerVariables.multiplier)
        engine.GetGame().GetHUD().UpdateUltReadyText(__playerVariables.ultCharge == __playerVariables.ultMaxCharge)

        var mousePosition = engine.GetInput().GetMousePosition()
        __playerMovement.lastMousePosition = mousePosition

        var playerPos = __player.GetTransformComponent().translation

        for (enemy in __enemyList) {

            // We delete the entity from the ecs when it dies
            // Then we check for entity validity, and remove it from the list if it is no longer valid
            if (enemy.entity.IsValid()) {
                enemy.Update(playerPos, __playerVariables, engine, dt)
            } else {
                __enemyList.removeAt(__enemyList.indexOf(enemy))
            }
        }

        __waveSystem.Update(dt)
    }
}
