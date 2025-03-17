import "engine_api.wren" for Engine, Input

class Main {

    static Start(engine) {
        engine.GetInput().SetActiveActionSet("UserInterface")

        System.print("Start main menu")
    }

    static Shutdown(engine) {
        System.print("Exited main menu")
    }

    static Update(engine, dt) {
        if (engine.GetInput().GetDigitalAction("CloseMenu").IsPressed()) {
            engine.TransitionToScript("game/game.wren")
            return
        }
    }
}
