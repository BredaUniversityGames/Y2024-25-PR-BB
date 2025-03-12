import "engine_api.wren" for Engine, Input, Vec3, Quat, Math

class Main {

    static Start(engine) {
        System.print("Start main menu")
        engine.GetInput().SetMouseHidden(false)
        engine.GetGame().SetMainMenuEnabled(true)
        __entities = engine.LoadModel("assets/models/main_menu.glb")
       
        __transform = __entities[0].GetTransformComponent()
        __transform.translation = Vec3.new(15.4,-20,-203)
        __transform.rotation = Quat.new(0,0,-0.2,0.1)
        
        var camera = engine.GetECS().GetEntityByName("Camera")
        var camTrans = camera.GetTransformComponent()
        camTrans.rotation = Quat.new(0.982,0.145,0.117,-0.017)
                 
        engine.GetECS().GetEntityByName("l1").AddPointLightComponent().color = Vec3.new(4, 0.4,0)
        engine.GetECS().GetEntityByName("l2").AddPointLightComponent().color = Vec3.new(4, 0.4,0)
        engine.GetECS().GetEntityByName("l3").AddPointLightComponent().color = Vec3.new(4, 0.4,0)
        engine.GetECS().GetEntityByName("l4").AddPointLightComponent().color = Vec3.new(4, 0.4,0)
        engine.GetECS().GetEntityByName("l5").AddPointLightComponent().color = Vec3.new(4, 0.4,0)
        engine.GetECS().GetEntityByName("l6").AddPointLightComponent().color = Vec3.new(4, 0.4,0)
        engine.GetECS().GetEntityByName("l7").AddPointLightComponent().color = Vec3.new(30,3,3)
    }

    static Shutdown(engine) {
        System.print("Exited main menu")
   
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