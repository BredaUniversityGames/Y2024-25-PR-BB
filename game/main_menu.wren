import "engine_api.wren" for Engine, Input, Vec3, Vec2, Quat, Math, Keycode, Random, Perlin
import "gameplay/camera.wren" for CameraVariables
import "gameplay/music_player.wren" for MusicPlayer

class Main {

    static Start(engine) {
        System.print("Start main menu")

        engine.Fog = -0.02

        engine.GetInput().SetActiveActionSet("UserInterface")
        engine.GetInput().SetMouseHidden(false)
        engine.GetGame().SetMainMenuEnabled(true)
        engine.GetUI().SetSelectedElement(engine.GetGame().GetMainMenu().playButton)
        
        var helmet = engine.LoadModel("assets/models/plague_helmet.glb")
        helmet.GetTransformComponent().translation = Vec3.new(8.6, 1.2, -19.8)
        helmet.GetTransformComponent().rotation = Math.ToQuat(Vec3.new(0.0, -0.471239, 0.0))
        helmet.GetTransformComponent().scale = Vec3.new(1.5, 1.5, 1.5)

        var light = engine.GetECS().NewEntity()
        light.AddNameComponent().name = "Helmet point light"
        __lightComponent = light.AddPointLightComponent()
        __lightComponent.color = Vec3.new(220 / 255, 50 / 255, 50 / 255)
        __lightComponent.range = 35
        __lightComponent.intensity = 39
        
        light.AddTransformComponent().translation = Vec3.new(4.8, 4.7, -10.6) // range: 91, intensity: 20

        // __background = engine.LoadModel("assets/models/main_menu.glb")
       
        // __transform = __background.GetTransformComponent()
        // __transform.translation = Vec3.new(15.4,-20,-203)
        // __transform.rotation = Quat.new(0,0,-0.2,0.1)
        
        __camera = engine.GetECS().NewEntity()
        __cameraVariables = CameraVariables.new()
        
        var cameraProperties = __camera.AddCameraComponent()
        cameraProperties.fov = Math.Radians(28.0)
        cameraProperties.nearPlane = 0.5
        cameraProperties.farPlane = 600.0
        cameraProperties.reversedZ = true

        __camera.AddTransformComponent()
        __camera.AddAudioEmitterComponent()
        __camera.AddNameComponent().name = "Camera"
        __camera.AddAudioListenerTag()

        var camTrans = __camera.GetTransformComponent()
        camTrans.translation = Vec3.new(5.3, -8, 29.1)
        camTrans.rotation = Quat.new(0.982,0.145,0.117,-0.017)

        __directionalLight = engine.GetECS().NewEntity()
        __directionalLight.AddNameComponent().name = "Directional Light"

        var directionalLightComp = __directionalLight.AddDirectionalLightComponent()
        directionalLightComp.color = Vec3.new(4.0, 3.2, 1.2)
        directionalLightComp.planes = Vec2.new(-30.0, 30.0)
        directionalLightComp.orthographicSize = 30.0

        var transform = __directionalLight.AddTransformComponent()
        transform.rotation = Math.ToQuat(Vec3.new(Math.Radians(144), Math.Radians(63), Math.Radians(-178)))

        var settings = engine.GetGame().GetMainMenu().settingsButton
        settings.OnPress(Fn.new { System.print("Settings Opened!")})

        var play = engine.GetGame().GetMainMenu().playButton
        play.OnPress(Fn.new {
            engine.GetGame().SetMainMenuEnabled(false)
            engine.TransitionToScript("game/game.wren")
        })

        var exit = engine.GetGame().GetMainMenu().quitButton
        exit.OnPress(Fn.new {
            engine.SetExit(0)
        })
        
        var musicList = [
            "assets/music/main_menu/Alon Peretz - The Queens Quarters.wav",
            ""
            ]

        var ambientList = [
            "assets/music/ambient/Crow Cawing 2 - QuickSounds.com.mp3",
            ""
            ]

        __musicPlayer = MusicPlayer.new(engine.GetAudio(), musicList, 0.3)
        __ambientPlayer = MusicPlayer.new(engine.GetAudio(), ambientList, 0.1)

        __perlin = Perlin.new(0)
        __baseIntensity = 20.0
        __flickerRange = 46.0
        __flickerSpeed = 1.0
        __noiseOffset = 0.0
    }

    static Shutdown(engine) {
        System.print("Exited main menu")
        engine.GetECS().DestroyAllEntities()
        __musicPlayer.Destroy(engine.GetAudio())
        __ambientPlayer.Destroy(engine.GetAudio())
    }

    static Update(engine, dt) {
        __noiseOffset = __noiseOffset + dt * 0.001 * __flickerSpeed
        var noise = __perlin.Noise1D(__noiseOffset)
        var flickerIntensity = __baseIntensity + ((noise - 0.5) * __flickerRange)
        __lightComponent.intensity = flickerIntensity
    }
}
