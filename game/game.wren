import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, PhysicsObjectLayer, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/movement.wren" for PlayerMovement
import "gameplay/enemies/spawner.wren" for Spawner
import "gameplay/weapon.wren" for Pistol, Shotgun, Knife, Weapons
import "gameplay/camera.wren" for CameraVariables
import "gameplay/player.wren" for PlayerVariables, HitmarkerState
import "gameplay/music_player.wren" for MusicPlayer, BGMPlayer
import "gameplay/wave_system.wren" for WaveSystem, WaveConfig, SpawnLocationType
import "analytics/analytics.wren" for AnalyticsManager
import "gameplay/enemies/berserker_enemy.wren" for BerserkerEnemy

import "gameplay/enemies/ranged_enemy.wren" for RangedEnemy
import "gameplay/soul.wren" for Soul, SoulManager
import "gameplay/coin.wren" for Coin, CoinManager

class Main {

    static Start(engine) {

        engine.GetTime().SetScale(1.0)
        engine.GetInput().SetActiveActionSet("Shooter")
        engine.GetGame().SetUIMenu(engine.GetGame().GetHUD())

        engine.Fog = 0.005

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/blockoutv5navmesh_04.glb")
       
        // engine.GetAudio().LoadSFX("assets/sounds/slide2.wav", true, true)
        // engine.GetAudio().LoadSFX("assets/sounds/crows.wav", true, false)
        // engine.GetAudio().LoadSFX("assets/sounds/hitmarker.wav", false, false)
        // engine.GetAudio().LoadSFX("assets/sounds/hit1.wav", false, false)
        // engine.GetAudio().LoadSFX("assets/sounds/demon_roar.wav", true, false)
        // engine.GetAudio().LoadSFX("assets/sounds/shoot.wav", false, false)

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
        var rb = Rigidbody.new(engine.GetPhysics(), shape, PhysicsObjectLayer.ePLAYER(), false) // physics module, shape, layer, allowRotation
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
        engine.LoadModel("assets/models/graveyard_level.glb", true)

        engine.PreloadModel("assets/models/Skeleton.glb")
        engine.PreloadModel("assets/models/eye.glb")
        engine.PreloadModel("assets/models/Berserker.glb")

        engine.PreloadModel("assets/models/Revolver.glb")
        engine.PreloadModel("assets/models/Shotgun.glb")

        // Loading lights from gltf, uncomment to test
        // engine.LoadModel("assets/models/light_test.glb")

        // Gun Setup
        __gun = engine.LoadModel("assets/models/Revolver.glb",false)
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

        __enemyShape = ShapeFactory.MakeCapsuleShape(70.0, 70.0)
        __eyeShape = ShapeFactory.MakeSphereShape(0.65)
        __berserkerEnemyShape = ShapeFactory.MakeCapsuleShape(140.0, 50.0)

        // Music

        __musicPlayer = BGMPlayer.new(engine.GetAudio(),
            "event:/BGM/Gameplay",
            0.12)

        __ambientPlayer = BGMPlayer.new(engine.GetAudio(),
            "event:/BGM/DarkSwampAmbience",
            0.1)

        var spawnLocations = []
        for(i in 0..8) {
            var entity = engine.GetECS().GetEntityByName("Spawner_%(i)")
            if(entity) {
                spawnLocations.add(entity)
            }
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
            .AddSpawn("Eye", SpawnLocationType.Closest, 8, 1)
            .AddSpawn("Skeleton", 0, 10, 1)
            .AddSpawn("Skeleton", 1, 15, 1)
            .AddSpawn("Skeleton", 2, 5, 1)
            .AddSpawn("Skeleton", 3, 15, 3)
            .AddSpawn("Eye", SpawnLocationType.Closest, 25, 1)
        )
        waveConfigs.add(WaveConfig.new().SetDuration(60)
            .AddSpawn("Skeleton", 0, 1, 2)
            .AddSpawn("Skeleton", 1, 1, 2)
            .AddSpawn("Skeleton", 2, 1, 1)
            .AddSpawn("Skeleton", 3, 1, 2)
            .AddSpawn("Eye", SpawnLocationType.Closest, 1, 1)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 5, 5)
            .AddSpawn("Skeleton", 0, 15, 2)
            .AddSpawn("Skeleton", 1, 15, 1)
            .AddSpawn("Skeleton", 2, 15, 2)
            .AddSpawn("Skeleton", 3, 15, 3)
            .AddSpawn("Eye", SpawnLocationType.Closest, 5, 1)
            .AddSpawn("Eye", SpawnLocationType.Closest, 20, 1)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 15, 5)
            .AddSpawn("Skeleton", 0, 40, 3)
            .AddSpawn("Skeleton", 1, 40, 1)
            .AddSpawn("Skeleton", 2, 40, 2)
            .AddSpawn("Skeleton", 3, 40, 2)
            .AddSpawn("Skeleton", SpawnLocationType.Furthest, 7, 5)
        )
        __waveSystem = WaveSystem.new(engine, waveConfigs, __enemyList, spawnLocations, __player)

        // Souls
        __soulManager = SoulManager.new(engine, __player)
        __coinManager = CoinManager.new(engine, __player)

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
            __musicPlayer.SetVolume(engine.GetAudio(), 0.05)
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
    }

    static Shutdown(engine) {
        engine.ResetDecals()

        __musicPlayer.Destroy(engine.GetAudio())
        __ambientPlayer.Destroy(engine.GetAudio())

        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {

        // Check if pause key was pressed
        if(__alive && engine.GetInput().GetDigitalAction("Menu").IsPressed()) {

            __pauseEnabled = !__pauseEnabled

            if (__pauseEnabled) {
                __pauseHandler.call()
            } else {
                __unpauseHandler.call()
            }
        }

        // Skip everything if paused
        if (__pauseEnabled) {
            return
        }

        __playerMovement.lookSensitivity = engine.GetGame().GetSettings().aimSensitivity * (2.5 - 0.2) + 0.2

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
            engine.GetAudio().PlayEventOnce("event:/SFX/UltReady")
            __playerVariables.wasUltReadyLastFrame = true
        }

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

                    engine.GetAudio().PlayEventOnce("event:/SFX/ActivateUlt")

                    var particleEntity = engine.GetECS().NewEntity()
                    particleEntity.AddTransformComponent().translation = __player.GetTransformComponent().translation - Vec3.new(0,3.5,0)
                    var lifetime = particleEntity.AddLifetimeComponent()
                    lifetime.lifetime = 400.0
                    var emitterFlags = SpawnEmitterFlagBits.eIsActive()
                    engine.GetParticles().SpawnEmitter(particleEntity, EmitterPresetID.eHealth(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
                }
            }

            if (engine.GetInput().DebugGetKey(Keycode.eG()) && false) {
                if (__playerVariables.grenadeCharge == __playerVariables.grenadeMaxCharge) {
                    // Throw grenade
                    __playerVariables.grenadeCharge = 0
                }
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
                __activeWeapon.attack(engine, dt, __playerVariables, __enemyList, __coinManager)
                if (__activeWeapon.ammo <= 0) {
                    __activeWeapon.reload(engine)
                }
            }

            if (engine.GetInput().DebugGetKey(Keycode.eK())) {
                __enemyList.add(BerserkerEnemy.new(engine, Vec3.new(0, 18, 7), Vec3.new(0.026, 0.026, 0.026), 4, "assets/models/Berserker.glb", __berserkerEnemyShape))
            }

            if (engine.GetInput().DebugGetKey(Keycode.eJ())) {
                __enemyList.add(RangedEnemy.new(engine, Vec3.new(-27, 18, 7), Vec3.new(2.25,2.25,2.25), 5, "assets/models/eye.glb", __eyeShape))
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

        engine.GetGame().GetHUD().UpdateHealthBar(__playerVariables.health / __playerVariables.maxHealth)
        engine.GetGame().GetHUD().UpdateAmmoText(__activeWeapon.ammo, __activeWeapon.maxAmmo)
        engine.GetGame().GetHUD().UpdateUltBar(__playerVariables.ultCharge / __playerVariables.ultMaxCharge)
        engine.GetGame().GetHUD().UpdateScoreText(__playerVariables.score)
        engine.GetGame().GetHUD().UpdateGrenadeBar(__playerVariables.grenadeCharge / __playerVariables.grenadeMaxCharge)
        engine.GetGame().GetHUD().UpdateDashCharges(__playerMovement.currentDashCount)
        engine.GetGame().GetHUD().UpdateMultiplierText(__playerVariables.multiplier)
        engine.GetGame().GetHUD().ShowHitmarker(__playerVariables.hitmarkTimer > 0 && __playerVariables.hitmarkerState == HitmarkerState.normal)
        engine.GetGame().GetHUD().ShowHitmarkerCrit(__playerVariables.hitmarkTimer > 0 && __playerVariables.hitmarkerState == HitmarkerState.crit)
        engine.GetGame().GetHUD().UpdateUltReadyText(__playerVariables.ultCharge == __playerVariables.ultMaxCharge)

        var mousePosition = engine.GetInput().GetMousePosition()
        __playerMovement.lastMousePosition = mousePosition

        var playerPos = __playerController.GetRigidbodyComponent().GetPosition()

        if(engine.GetInput().DebugGetKey(Keycode.eB())){

            // Spawn between 1 and 5 coins
                var coinCount = Random.RandomIndex(1, 5)
                for(i in 0...coinCount) {
                              __coinManager.SpawnCoin(engine, Vec3.new(10.0,2.0,44.0))

                }
        }

        for (enemy in __enemyList) {

            // We delete the entity from the ecs when it dies
            // Then we check for entity validity, and remove it from the list if it is no longer valid
            if (enemy.entity.IsValid()) {
                enemy.Update(playerPos, __playerVariables, engine, dt, __soulManager, __coinManager)
            } else {
                __enemyList.removeAt(__enemyList.indexOf(enemy))
            }
        }

        __soulManager.Update(engine, __playerVariables, dt)
        __coinManager.Update(engine, __playerVariables, dt)
        __waveSystem.Update(dt)
    }
}
