import "engine_api.wren" for Engine, Input, Vec3, Vec2, Quat, Math, Keycode, Random, Perlin, ShapeFactory, Rigidbody, RigidbodyComponent, PhysicsObjectLayer
import "gameplay/camera.wren" for CameraVariables
import "gameplay/music_player.wren" for BGMPlayer

class Main {

    static Start(engine) {
        System.print("Start main menu")

        engine.Fog = -0.02
        engine.AmbientStrength = 1.25

        engine.GetInput().SetActiveActionSet("UserInterface")
        engine.GetInput().SetMouseHidden(false)

        engine.GetGame().SetUIMenu(engine.GetGame().GetMainMenu())
        engine.GetUI().SetSelectedElement(engine.GetGame().GetMainMenu().playButton)

        var helmet = engine.LoadModel("assets/models/plague_helmet.glb", false)
        helmet.GetTransformComponent().translation = Vec3.new(17.60, 1.4, -9.8)
        helmet.GetTransformComponent().rotation = Quat.new(0.926,-0.041,-0.376,-0.017)
        helmet.GetTransformComponent().scale = Vec3.new(1.6, 1.6, 1.6)

        var light = engine.GetECS().NewEntity()
        light.AddNameComponent().name = "Helmet point light"
        __lightComponent = light.AddPointLightComponent()
        __lightComponent.color = Vec3.new(220 / 255, 50 / 255, 50 / 255)
        __lightComponent.range = 35
        __lightComponent.intensity = 39

        var ambientLight = engine.GetECS().NewEntity()


        ambientLight.AddNameComponent().name = "Helmet point ambientLight"
        var ambientLightComponent = ambientLight.AddPointLightComponent()
        ambientLightComponent.color = Vec3.new(220 / 255, 50 / 255, 50 / 255)
        ambientLightComponent.range = 41
        ambientLightComponent.intensity = 10

        ambientLight.AddTransformComponent().translation = Vec3.new(11.900, 5.70, -2.4) // range: 91, intensity: 20


        light.AddTransformComponent().translation = Vec3.new(11.900, 5.70, -2.4) // range: 91, intensity: 20

        __camera = engine.GetECS().NewEntity()
        __cameraVariables = CameraVariables.new()

        var cameraProperties = __camera.AddCameraComponent()
        cameraProperties.fov = Math.Radians(40.0)
        cameraProperties.nearPlane = 0.5
        cameraProperties.farPlane = 600.0
        cameraProperties.reversedZ = true

        __camera.AddTransformComponent()
        __camera.AddAudioEmitterComponent()
        __camera.AddNameComponent().name = "Camera"
        __camera.AddAudioListenerTag()

        var camTrans = __camera.GetTransformComponent()
        camTrans.translation = Vec3.new(5.3, -0.3, 29.1)
        camTrans.rotation = Quat.new(0.990,0.079,0.115,-0.009)

        __directionalLight = engine.GetECS().NewEntity()
        __directionalLight.AddNameComponent().name = "Directional Light"

        var directionalLightComp = __directionalLight.AddDirectionalLightComponent()
        directionalLightComp.color = Vec3.new(4.0, 3.2, 1.2)
        directionalLightComp.planes = Vec2.new(-30.0, 30.0)
        directionalLightComp.orthographicSize = 30.0

        var transform = __directionalLight.AddTransformComponent()
        transform.rotation = Math.ToQuat(Vec3.new(Math.Radians(161.1), Math.Radians(77), Math.Radians(-178)))

        var play = engine.GetGame().GetMainMenu().playButton
        play.OnPress(Fn.new {
            engine.TransitionToScript("game/loading_screen.wren")
        })

        var exit = engine.GetGame().GetMainMenu().quitButton
        exit.OnPress(Fn.new {
            engine.SetExit(0)
        })

        engine.GetGame().GetGameVersionVisual().Show(true)

        __musicPlayer = BGMPlayer.new(engine.GetAudio(),
            "event:/BGM/MainMenu",
            0.3)
        __ambientPlayer = BGMPlayer.new(engine.GetAudio(),
            "event:/SFX/CrowCrawling",
            0.1)

        __perlin = Perlin.new(0)
        __baseIntensity = 30.0
        __flickerRange = 56.0
        __flickerSpeed = 1.0
        __noiseOffset = 0.0
    }

    static Shutdown(engine) {
        System.print("Exited main menu")
        __musicPlayer.Destroy(engine.GetAudio())
        __ambientPlayer.Destroy(engine.GetAudio())
        engine.GetGame().GetGameVersionVisual().Show(false)
    }

    static Update(engine, dt) {
        __noiseOffset = __noiseOffset + dt * 0.001 * __flickerSpeed
        var noise = __perlin.Noise1D(__noiseOffset)
        var flickerIntensity = __baseIntensity + ((noise - 0.5) * __flickerRange)
        __lightComponent.intensity = flickerIntensity
    }
}
