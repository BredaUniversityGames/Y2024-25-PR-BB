import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, PhysicsObjectLayer, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, Random
import "gameplay/movement.wren" for PlayerMovement
import "gameplay/weapon.wren" for Pistol, Shotgun, Weapons
import "gameplay/camera.wren" for CameraVariables
import "gameplay/player.wren" for PlayerVariables, HitmarkerState
import "gameplay/music_player.wren" for MusicPlayer, BGMPlayer
import "gameplay/wave_system.wren" for WaveSystem, WaveConfig, EnemyType, WaveGenerator
import "analytics/analytics.wren" for AnalyticsManager

import "gameplay/hud.wren" for WrenHUD
import "gameplay/soul.wren" for Soul, SoulManager, SoulType
import "gameplay/coin.wren" for Coin, CoinManager
import "gameplay/flash_system.wren" for FlashSystem
import "gameplay/station.wren" for PowerUpType, Station, StationManager
import "gameplay/powerup_system.wren" for PowerUpSystem
import "debug_utils.wren" for DebugUtils

class Main {

    static Start(engine) {

        engine.GetTime().SetScale(1.0)
        engine.GetInput().SetActiveActionSet("Shooter")
        engine.GetGame().SetUIMenu(engine.GetGame().GetHUD())

        engine.Fog = 0.005
        engine.AmbientStrength = 0.35

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/graveyard_navmesh_001.glb")

        // Directional Light
        __directionalLight = engine.GetECS().NewEntity()
        __directionalLight.AddNameComponent().name = "Directional Light"

        var comp = __directionalLight.AddDirectionalLightComponent()
        comp.color = Vec3.new(4.0, 3.2, 1.2).mulScalar(0.15)
        comp.planes = Vec2.new(-50.0, 1000.0)
        comp.orthographicSize = 120.0

        var transform = __directionalLight.AddTransformComponent()
        transform.translation = Vec3.new(-74.000, 134.800, 156.900)
        transform.rotation = Quat.new(0.559, -0.060, -0.821,-0.101)

        // Player Setup

        __playerVariables = PlayerVariables.new(engine.GetGame().GetHUD(), engine)
        __counter = 0
        __frameTimer = 0
        __groundedTimer = 0
        __hasDashed = false
        __timer = 0

        // Load Map
        engine.LoadModel("assets/models/graveyard_level.glb", false)
        engine.LoadCollisions("assets/models/graveyard_collisions.glb")

        engine.PreloadModel("assets/models/Skeleton.glb")
        engine.PreloadModel("assets/models/eye.glb")
        engine.PreloadModel("assets/models/Berserker.glb")

        engine.PreloadModel("assets/models/Revolver.glb")
        engine.PreloadModel("assets/models/Shotgun.glb")

        // Player stuff
        engine.GetInput().SetMouseHidden(true)
        __playerController = engine.GetECS().NewEntity()
        __camera = engine.GetECS().NewEntity()
        __player = engine.GetECS().NewEntity()

        var playerTransform = __playerController.AddTransformComponent()
        var playerStart = engine.GetECS().GetEntityByName("PlayerStart")
        var playerStartPos = Vec3.new(0.0, 0.0, 0.0)

        if(playerStart) {
            playerTransform.translation = playerStart.GetTransformComponent().translation
            playerTransform.rotation = playerStart.GetTransformComponent().rotation
            playerStartPos = playerStart.GetTransformComponent().translation
        }

        __playerController.AddPlayerTag()
        __playerController.AddNameComponent().name = "PlayerController"
        __playerController.AddCheatsComponent().noClip = false

        var shape = ShapeFactory.MakeCapsuleShape(1.7, 0.5) // height, circle radius
        var rb = Rigidbody.new(engine.GetPhysics(), shape, PhysicsObjectLayer.ePLAYER(), false)
        __playerController.AddRigidbodyComponent(rb)

        __cameraVariables = CameraVariables.new()
        __playerVariables.cameraVariables = __cameraVariables
        var cameraProperties = __camera.AddCameraComponent()
        cameraProperties.fov = engine.GetGame().GetSettings().fov // Get from where we manage fov

        cameraProperties.nearPlane = 0.1
        cameraProperties.farPlane = 600.0
        cameraProperties.reversedZ = true

        __camera.AddTransformComponent()
        __camera.AddAudioEmitterComponent()
        __camera.AddNameComponent().name = "Camera"
        __camera.AddAudioListenerTag()

        __player.AddTransformComponent().translation = playerTransform.translation
        __player.GetTransformComponent().rotation = playerTransform.rotation
        __player.AddNameComponent().name = "Player"

        // Gun Setup
        __gunParentPivot = engine.GetECS().NewEntity()
        __gunParentPivot.AddNameComponent().name = "GunParentPivot"
        __gunParentPivot.AddTransformComponent().translation = Vec3.new(3.0, 0.0 , 0.0)

        __gunParentPivot2 = engine.GetECS().NewEntity()
        __gunParentPivot2.AddNameComponent().name = "GunParentPivot2"
        __gunParentPivot2.AddTransformComponent().translation = Vec3.new(3.0, 0.0 , 0.0)

        __gunPivot = engine.GetECS().NewEntity()
        __gunPivot.AddNameComponent().name = "GunPivot"

        __gunPivot2 = engine.GetECS().NewEntity()
        __gunPivot2.AddNameComponent().name = "GunPivot2"

        var gunPivotTransform = __gunPivot.AddTransformComponent()
        gunPivotTransform.translation = Vec3.new(-3.0, 0.0 , 0.0)

        var gunPivotTransform2 = __gunPivot2.AddTransformComponent()
        gunPivotTransform2.translation = Vec3.new(-3.0, 0.0 , 0.0)

        __gun = engine.LoadModel("assets/models/Revolver.glb",false)
        __gun.RenderInForeground()

        __gun.GetNameComponent().name = "Gun"

        var gunTransform = __gun.GetTransformComponent()
        gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI()/2, 0.0))

        // second gun
        __gun2 = engine.LoadModel("assets/models/revolver_left.glb",false)
        __gun2.RenderInForeground()

        __gun2.GetNameComponent().name = "Gun2"

        var gunTransform2 = __gun2.GetTransformComponent()
        gunTransform2.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI()/2, 0.0))
        //gunTransform2.translation = gunTransform2.translation * Vec3.new(1, 1, 1)
        gunTransform2.scale = Vec3.new(0,0,0) // scale to 0 to hide

        __player.AttachChild(__camera)
        __camera.AttachChild(__gunParentPivot)
        __gunParentPivot.AttachChild(__gunPivot)
        __gunPivot.AttachChild(__gun)

        __camera.AttachChild(__gunParentPivot2)
        __gunParentPivot2.AttachChild(__gunPivot2)
        __gunPivot2.AttachChild(__gun2)

        __armory = [Pistol.new(engine, "Gun",Vec3.new(-2.5,-0.35,-0.85)), Shotgun.new(engine), Pistol.new(engine, "Gun2",Vec3.new(-2.5,-0.35,-0.85))]

        __armory[2].playWalkAnim(engine)

        __activeWeapon = __armory[Weapons.pistol]
        __activeWeapon.equip(engine)
        __nextWeapon = null

        __secondaryWeapon = __armory[Weapons.pistol2]

        // create the player movement
        __playerMovement = PlayerMovement.new(false,0.0,__activeWeapon,__player, playerStartPos, __playerVariables)
        var mousePosition = engine.GetInput().GetMousePosition()
        __playerMovement.lastMousePosition = mousePosition

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)

        //__ultimateCharge = 0
        //__ultimateActive = false

        __pauseEnabled = false

        // Music

        __musicPlayer = BGMPlayer.new(engine.GetAudio(),
            "event:/BGM/Gameplay",
            0.10)

        __ambientPlayer = BGMPlayer.new(engine.GetAudio(),
            "event:/BGM/DarkSwampAmbience",
            0.1)

        __heartBeatSFX = "event:/SFX/HeartBeat"
        __heartBeatEvent = engine.GetAudio().PlayEventLoop(__heartBeatSFX)

        __camera.GetAudioEmitterComponent().AddEvent(__heartBeatEvent)

        var spawnLocations = []
        for(i in 0..8) {
            var entity = engine.GetECS().GetEntityByName("Spawner_%(i)")
            if(entity) {
                spawnLocations.add(entity.GetTransformComponent().translation)
            }
        }

        __enemyList = []

        var waveConfigs = []

        for (v in 0...30) {
            waveConfigs.add(WaveGenerator.GenerateWave(v))
        }

        __waveSystem = WaveSystem.new(waveConfigs, spawnLocations)

        // Souls
        __soulManager = SoulManager.new(engine, __player)

        // Coins
        __coinManager = CoinManager.new(engine, __player)

        // Power ups
        __stationManager = StationManager.new(engine, __player)
        __powerUpSystem = PowerUpSystem.new(engine)

        // Flash System
        __flashSystem = FlashSystem.new(engine)

        // Pause Menu callbacks

        __pauseHandler = Fn.new {
            __pauseEnabled = true
            engine.GetTime().SetScale(0.0)

            engine.GetInput().SetMouseHidden(false)
            engine.GetGame().PushUIMenu(engine.GetGame().GetPauseMenu())
            engine.GetInput().SetActiveActionSet("UserInterface")

            engine.GetUI().SetSelectedElement(engine.GetGame().GetPauseMenu().continueButton)
            __musicPlayer.SetVolume(engine.GetAudio(), 0.05)
            System.print("Pause Menu is %(__pauseEnabled)!")
        }

        __unpauseHandler = Fn.new {
            __pauseEnabled = false
            engine.GetTime().SetScale(1.0)

            engine.GetGame().SetUIMenu(engine.GetGame().GetHUD())
            engine.GetInput().SetActiveActionSet("Shooter")
            engine.GetInput().SetMouseHidden(true)
            __musicPlayer.SetVolume(engine.GetAudio(), 0.10)
            System.print("Pause Menu is %(__pauseEnabled)!")
        }

        var continueButton = engine.GetGame().GetPauseMenu().continueButton
        continueButton.OnPress(__unpauseHandler)

        var backToMain = Fn.new {
            engine.TransitionToScript("game/main_menu.wren")
            engine.GetTime().SetScale(1.0)
        }

        var menuButton = engine.GetGame().GetPauseMenu().backButton
        menuButton.OnPress(backToMain)

        // Game over callbacks
        __alive = true

        var menuButton2 = engine.GetGame().GetGameOverMenu().backButton
        menuButton2.OnPress(backToMain)

        var retryButton = engine.GetGame().GetGameOverMenu().retryButton

        var retryHandler = Fn.new {
            engine.TransitionToScript("game/game.wren")
            engine.GetTime().SetScale(1.0)
        }

        retryButton.OnPress(retryHandler)

        __gamepadConnectedPrevFrame = engine.GetInput().IsGamepadConnected()
    }

    static Shutdown(engine) {
        __musicPlayer.Destroy(engine.GetAudio())
        __ambientPlayer.Destroy(engine.GetAudio())
    }

    static Update(engine, dt) {

        // Pausing functionality
        if(__alive) {
            // Pause the game if the controller disconnects
            var hasGamepadDisconnected = __gamepadConnectedPrevFrame && !engine.GetInput().IsGamepadConnected()
            __gamepadConnectedPrevFrame = engine.GetInput().IsGamepadConnected()

            if (!__pauseEnabled && hasGamepadDisconnected) {
                __pauseHandler.call()
            }

            // Check if pause key was pressed
            var pausePressed = false
            if (!__pauseEnabled) {
                pausePressed = engine.GetInput().GetDigitalAction("Pause").IsReleased()
            } else {
                pausePressed = engine.GetInput().GetDigitalAction("Unpause").IsReleased()
            }

            if (pausePressed) {
                __pauseEnabled = !__pauseEnabled

                if(__pauseEnabled) {
                    __pauseHandler.call()
                } else {
                    __unpauseHandler.call()
                }
            }
        }


        // Update fov
        __playerMovement.UpdateFOV(50 + 100 * engine.GetGame().GetSettings().fov)
        __camera.GetCameraComponent().fov = Math.Radians(__playerMovement.cameraFovCurrent)

        // Skip everything if paused
        if (__pauseEnabled || !__alive) {
            __camera.GetCameraComponent().fov = Math.Radians(50 + 100 * engine.GetGame().GetSettings().fov)

            var mousePosition = engine.GetInput().GetMousePosition()
            __playerMovement.lastMousePosition = mousePosition

            return
        }

        __playerMovement.lookSensitivity = engine.GetGame().GetSettings().aimSensitivity * (2.5 - 0.2) + 0.2

        if (__enemyList.count != 0) {
            __musicPlayer.SetAttribute(engine.GetAudio(), "Intensity", 1.0)
        } else {
            __musicPlayer.SetAttribute(engine.GetAudio(), "Intensity", 0.0)
        }

        var healthFraction = __playerVariables.health / __playerVariables.maxHealth
        engine.GetAudio().SetEventFloatAttribute(__heartBeatEvent, "Health", healthFraction)
        if (healthFraction < 0.3) {
            __flashSystem.SetBaseColor(Vec3.new(105 / 255, 13 / 255, 1 / 255),1 - healthFraction*4)
            engine.GetAudio().EnableLowPass()
        } else {
            engine.GetAudio().DisableLowPass()
        }



        var cheats = __playerController.GetCheatsComponent()
        var deltaTime = engine.GetTime().GetDeltatime()
        __timer = __timer + dt
        __playerVariables.grenadeCharge = Math.Min(__playerVariables.grenadeCharge + __playerVariables.grenadeChargeRate * dt / 1000, __playerVariables.grenadeMaxCharge)

        // if (__playerVariables.ultActive) {
        //     __playerVariables.ultCharge = Math.Max(__playerVariables.ultCharge - __playerVariables.ultDecayRate * dt / 1000, 0)
        //     if (__playerVariables.ultCharge <= 0) {
        //         __activeWeapon = __armory[Weapons.pistol]
        //         __activeWeapon.equip(engine)
        //         __playerVariables.ultActive = false
        //         __playerVariables.wasUltReadyLastFrame = false
        //     }
        // }

        // if (!__playerVariables.wasUltReadyLastFrame && __playerVariables.ultCharge == __playerVariables.ultMaxCharge) {
        //     engine.GetAudio().PlayEventOnce("event:/SFX/UltReady")
        //     __playerVariables.wasUltReadyLastFrame = true
        // }

        __playerVariables.invincibilityTime = Math.Max(__playerVariables.invincibilityTime - dt, 0)
        __playerVariables.multiplierTimer = Math.Max(__playerVariables.multiplierTimer - dt, 0)
        __playerVariables.hitmarkTimer = Math.Max(__playerVariables.hitmarkTimer - dt, 0)

        __cameraVariables.Shake(engine, __camera, dt)
        __cameraVariables.Tilt(engine, __camera, dt)
        __cameraVariables.ProcessRecoil(engine, __camera, dt)

        if (__playerVariables.multiplierTimer == 0 ) {
            __playerVariables.multiplier = 1.0
            __playerVariables.consecutiveHits = 0
        }

        if (engine.GetInput().DebugIsInputEnabled()) {
            __playerMovement.Update(engine, dt, __playerController, __camera,__playerVariables.hud, __flashSystem)
        }
        var mousePosition = engine.GetInput().GetMousePosition()
        __playerMovement.lastMousePosition = mousePosition


        for (weapon in __armory) {
            weapon.cooldown = Math.Max(weapon.cooldown - dt, 0)

            weapon.reloadTimer = Math.Max(weapon.reloadTimer - dt, 0)
            if (weapon != __activeWeapon && __playerVariables.GetCurrentPowerUp() != PowerUpType.DOUBLE_GUNS) {
                if (weapon.reloadTimer <= 0) {
                    weapon.ammo = weapon.maxAmmo
                }
            }
        }

        if (engine.GetInput().DebugIsInputEnabled()) {

            if(engine.GetInput().DebugGetKey(Keycode.eN())){
               cheats.noClip = !cheats.noClip
            }

            // // engine.GetInput().GetDigitalAction("Ultimate").IsPressed()
            // if (engine.GetInput().DebugGetKey(Keycode.eU())) {
            //     if (__playerVariables.ultCharge >= __playerVariables.ultMaxCharge) {
            //         System.print("Activate ultimate")
            //         __activeWeapon = __armory[Weapons.shotgun]
            //         __activeWeapon.equip(engine)
            //         __playerVariables.ultActive = true

            //         engine.GetAudio().PlayEventOnce("event:/SFX/ActivateUlt")

            //         var particleEntity = engine.GetECS().NewEntity()
            //         particleEntity.AddTransformComponent().translation = __player.GetTransformComponent().translation - Vec3.new(0,3.5,0)
            //         var lifetime = particleEntity.AddLifetimeComponent()
            //         lifetime.lifetime = 400.0
            //         var emitterFlags = SpawnEmitterFlagBits.eIsActive()
            //         engine.GetParticles().SpawnEmitter(particleEntity, "Health", emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
            //     }
            // }

            if (engine.GetInput().DebugGetKey(Keycode.eG()) && false) {
                if (__playerVariables.grenadeCharge == __playerVariables.grenadeMaxCharge) {
                    // Throw grenade
                    __playerVariables.grenadeCharge = 0
                }
            }

            // if (engine.GetInput().DebugGetKey(Keycode.e1()) && __activeWeapon.isUnequiping(engine) == false) {
            //     __activeWeapon.unequip(engine)
            //     __nextWeapon = __armory[Weapons.pistol]
            // }

            // if (engine.GetInput().DebugGetKey(Keycode.e2()) && __activeWeapon.isUnequiping(engine) == false) {
            //     __activeWeapon.unequip(engine)
            //     __nextWeapon = __armory[Weapons.shotgun]
            // }
        }

        if(__activeWeapon.isUnequiping(engine) == false && __nextWeapon != null){
            __activeWeapon = __nextWeapon
            __nextWeapon = null
            __activeWeapon.equip(engine)
        }

        if (engine.GetInput().GetDigitalAction("Reload").IsHeld() && __activeWeapon.isUnequiping(engine) == false) {
            __activeWeapon.reload(engine)

            if (__playerVariables.GetCurrentPowerUp() == PowerUpType.DOUBLE_GUNS){
                __secondaryWeapon.reload(engine)
            }
        }

        if (engine.GetInput().GetDigitalAction("Shoot").IsHeld()  && __activeWeapon.isUnequiping(engine) == false ) {
            __activeWeapon.attack(engine, dt, __playerVariables, __enemyList, __coinManager, __playerMovement.cameraFovCurrent)
            if (__activeWeapon.ammo <= 0) {
                __activeWeapon.reload(engine)
            }

            if(__secondaryWeapon.ammo <= 0 && __playerVariables.GetCurrentPowerUp() == PowerUpType.DOUBLE_GUNS){
                __secondaryWeapon.reload(engine)
            }
        }

        if (engine.GetInput().GetDigitalAction("ShootSecondary").IsHeld()  && __activeWeapon.isUnequiping(engine) == false ) {

            if (__playerVariables.GetCurrentPowerUp() == PowerUpType.DOUBLE_GUNS){
                __secondaryWeapon.attack(engine, dt, __playerVariables, __enemyList, __coinManager, __playerMovement.cameraFovCurrent)
                if (__secondaryWeapon.ammo <= 0) {
                    __secondaryWeapon.reload(engine)
                }
            }
        }

        if (engine.GetGame().GetSettings().aimAssist) {
            __activeWeapon.rotateToTarget(engine)
            if (__playerVariables.GetCurrentPowerUp() == PowerUpType.DOUBLE_GUNS){
                __secondaryWeapon.rotateToTarget(engine)
            }

        }

        // Check if player died
        if (__alive && __playerVariables.health <= 0) {
            __alive = false
            engine.GetTime().SetScale(0.0)

            engine.GetGame().PushUIMenu(engine.GetGame().GetGameOverMenu())
            engine.GetInput().SetActiveActionSet("UserInterface")
            engine.GetInput().SetMouseHidden(false)

            engine.GetUI().SetSelectedElement(engine.GetGame().GetGameOverMenu().retryButton)
        }


        var playerPos = __playerController.GetRigidbodyComponent().GetPosition()
        for (enemy in __enemyList) {

            // We delete the entity from the ecs when it dies
            // Then we check for entity validity, and remove it from the list if it is no longer valid
            if (enemy.entity.IsValid()) {
                enemy.Update(playerPos, __playerVariables, engine, dt, __soulManager, __coinManager, __flashSystem)
            } else {
                __enemyList.removeAt(__enemyList.indexOf(enemy))
            }
        }

        __soulManager.Update(engine, __playerVariables,__flashSystem, dt)
        __coinManager.Update(engine, __playerVariables,__flashSystem, dt)
        __waveSystem.Update(engine, __player, __enemyList, dt,__playerVariables)

        __stationManager.Update(engine, __playerVariables, dt)
        __flashSystem.Update(engine, dt)
        __powerUpSystem.Update(engine,__playerVariables,__flashSystem, dt)

        __playerVariables.hud.Update(engine, dt,__playerMovement,__playerVariables,__activeWeapon.ammo, __activeWeapon.maxAmmo)

        if (!engine.IsDistribution()) {
            DebugUtils.Tick(engine, __enemyList,__soulManager, __coinManager, __flashSystem, __waveSystem, __playerVariables)
        }
    }
}
