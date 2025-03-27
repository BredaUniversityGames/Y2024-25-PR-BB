import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/movement.wren" for PlayerMovement
import "gameplay/enemies/spawner.wren" for Spawner
import "gameplay/weapon.wren" for Pistol, Shotgun, Knife, Weapons
import "gameplay/camera.wren" for CameraVariables
import "gameplay/player.wren" for PlayerVariables
import "gameplay/music_player.wren" for MusicPlayer
import "analytics/analytics.wren" for AnalyticsManager

class Main {

    static Start(engine) {
        engine.GetInput().SetActiveActionSet("Shooter")
        engine.GetGame().SetHUDEnabled(true)

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/blockoutv5navmesh.glb")

        // Loading sounds
        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")
        engine.GetAudio().LoadSFX("assets/sounds/slide2.wav", true, true)


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

        var positions = [Vec3.new(10.0, 8.4, 11.4), Vec3.new(13.4, -0.6, 73.7), Vec3.new(24.9, -0.6, 72.3), Vec3.new(-30, 7.8, -10.2), Vec3.new(-41, 6.9, 1.2), Vec3.new(42.1, 12.4, -56.9)]

        // Load Map
        engine.LoadModel("assets/models/blockoutv5.glb")

        engine.PreloadModel("assets/models/Demon.glb")

        // Loading lights from gltf, uncomment to test
        // engine.LoadModel("assets/models/light_test.glb")

        // Gun Setup
        __gun = engine.LoadModel("assets/models/Revolver.glb")

        __gunAnchor = engine.GetECS().NewEntity()
        __gunAnchor.AddTransformComponent().translation = Vec3.new(-0.4, -3.1, -1)
        __gunAnchor.AddNameComponent().name = "GunAnchor"

        __gun.GetNameComponent().name = "Gun"
        var gunTransform = __gun.GetTransformComponent()
        gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI()/2, 0.0))


        __player.AttachChild(__camera)
        __camera.AttachChild(__gunAnchor)
        __gunAnchor.AttachChild(__gun)

        __armory = [Pistol.new(engine), Shotgun.new(engine), Knife.new(engine)]

        __activeWeapon = __armory[Weapons.pistol]
        __activeWeapon.equip(engine)

        // create the player movement
        __playerMovement = PlayerMovement.new(false,0.0,__activeWeapon)

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)

        __ultimateCharge = 0
        __ultimateActive = false
        
        __pauseEnabled = false

        // Enemy setup
        __enemyList = []
        __spawnerList = []

        for (position in positions) {
            __spawnerList.add(Spawner.new(position, 7000.0))
        }

        __enemyShape = ShapeFactory.MakeCapsuleShape(70.0, 70.0)

        __spawnerList[0].SpawnEnemies(engine, __enemyList, Vec3.new(0.02, 0.02, 0.02), 5, "assets/models/demon.glb", __enemyShape, 1)

        // Music player
        var musicList = [
            "assets/music/game/Juval - Play Your Game - No Lead Vocals.wav",
            "assets/music/game/Ace - Silent Treatment.wav",
            "assets/music/game/Dono - Zero Gravity.wav",
            "assets/music/game/Ikoliks - Metal Warrior.wav",
            "assets/music/game/Tomáš Herudek - Smash Your Enemies.wav",
            "assets/music/game/Taheda - Phenomena.wav",
            ""
            ]

        var ambientList = [
            "assets/music/ambient/207841__speedenza__dark-swamp-theme-1.wav",
            "assets/music/ambient/749939__universfield__horror-background-atmosphere-10.mp3",
            "assets/music/ambient/759816__newlocknew__ambfant_a-mysterious-fairy-tale-forest-in-the-mountains.mp3",
            ""
            ]
            
        __musicPlayer = MusicPlayer.new(engine.GetAudio(), musicList, 0.0)
        __ambientPlayer = MusicPlayer.new(engine.GetAudio(), ambientList, 0.0)
    }

    static Shutdown(engine) {
        __musicPlayer.Destroy(engine.GetAudio())
        __ambientPlayer.Destroy(engine.GetAudio())
        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {


        // for (spawner in __spawnerList) {
        //     spawner.Update(engine, __enemyList, Vec3.new(0.02, 0.02, 0.02), 5, "assets/models/demon.glb", __enemyShape, dt)
        // }

        if (engine.GetInput().DebugGetKey(Keycode.e9())) {
            System.print("Next Ambient Track")
            __ambientPlayer.CycleMusic(engine.GetAudio())
        }

        if (engine.GetInput().DebugGetKey(Keycode.e8())) {
            System.print("Next Gameplay Track")
            __musicPlayer.CycleMusic(engine.GetAudio())
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
            }
        } else {
            __playerVariables.ultCharge = Math.Min(__playerVariables.ultCharge + __playerVariables.ultChargeRate * dt / 1000, __playerVariables.ultMaxCharge)
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
            
            // TODO: Pause Menu on ESC
            // if(engine.GetInput().DebugGetKey(Keycode.eESCAPE())) {
            //     __pauseEnabled = !__pauseEnabled

            //     if (__pauseEnabled) {
            //         engine.GetTime().SetScale(0.0)
            //     } else {
            //         engine.GetTime().SetScale(1.0)
            //     }
            // }
        }

        engine.GetGame().GetHUD().UpdateHealthBar(__playerVariables.health / __playerVariables.maxHealth)
        engine.GetGame().GetHUD().UpdateAmmoText(__activeWeapon.ammo, __activeWeapon.maxAmmo)
        engine.GetGame().GetHUD().UpdateUltBar(__playerVariables.ultCharge / __playerVariables.ultMaxCharge)
        engine.GetGame().GetHUD().UpdateScoreText(__playerVariables.score)
        engine.GetGame().GetHUD().UpdateGrenadeBar(__playerVariables.grenadeCharge / __playerVariables.grenadeMaxCharge)
        engine.GetGame().GetHUD().UpdateDashCharges(__playerMovement.currentDashCount)

        var mousePosition = engine.GetInput().GetMousePosition()
        __playerMovement.lastMousePosition = mousePosition

        var playerPos = __player.GetTransformComponent().translation

        for (enemy in __enemyList) {

            // We delete the entity from the ecs when it dies
            // Then we check for entity validity, and remove it from the list if it is no longer valid
            if (enemy.entity.IsValid()) {
                enemy.Update(playerPos, engine, dt)
            } else {
                __enemyList.removeAt(__enemyList.indexOf(enemy))
            }

        }
    }
}
