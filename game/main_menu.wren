import "engine_api.wren" for Engine, Input

class Main {

    static Start(engine) {
        System.print("Start main menu")
    }

    static Shutdown(engine) {
        System.print("Exited main menu")
    }

    static Update(engine, dt) {
        if (engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            engine.TransitionToScript("game/game.wren")
            return
        }
    }
}