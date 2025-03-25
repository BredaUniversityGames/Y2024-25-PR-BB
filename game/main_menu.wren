import "engine_api.wren" for Engine, Input, Vec3, Quat, Math
import "gameplay/camera.wren" for CameraVariables
class Main {

    static Start(engine) {
        engine.GetInput().SetActiveActionSet("UserInterface")

        System.print("Start main menu")
        engine.GetInput().SetMouseHidden(false)
        engine.GetGame().SetMainMenuEnabled(true)
        // __background = engine.LoadModel("assets/models/main_menu.glb")
       
        // __transform = __background.GetTransformComponent()
        // __transform.translation = Vec3.new(15.4,-20,-203)
        // __transform.rotation = Quat.new(0,0,-0.2,0.1)
        
        __camera = engine.GetECS().NewEntity()
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

        var camTrans = __camera.GetTransformComponent()
        camTrans.rotation = Quat.new(0.982,0.145,0.117,-0.017)

    }

    static Shutdown(engine) {
        System.print("Exited main menu")
        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {
       if(engine.GetGame().GetMainMenu().PlayButtonPressedOnce()){
            engine.GetGame().SetMainMenuEnabled(false)
            engine.TransitionToScript("game/game.wren")
            return
       }
	   
       if(engine.GetGame().GetMainMenu().QuitButtonPressedOnce()){
            engine.SetExit(0)
            return
      }
    }
}
