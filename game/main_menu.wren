import "engine_api.wren" for Engine, Input

class Main {

    static Start(engine) {
        System.print("Start main menu")
        engine.GetInput().SetMouseHidden(false)
        engine.GetGame().SetMainMenuVisible(true)
        
    }

    static Shutdown(engine) {
        System.print("Exited main menu")
    }

    static Update(engine, dt) {
       if(engine.GetGame().GetMainMenu().PlayButtonPressedOnce()){
            engine.GetGame().SetMainMenuVisible(false)
            engine.TransitionToScript("game/game.wren")
            return
       }
    }
}