import "engine_api.wren" for Engine, Vec4

class Main {

    static Start(engine) {
        System.print("Start loading screen")

        __loadingScreen = engine.GetGame().GetLoadingScreen()
        engine.GetGame().SetUIMenu(engine.GetGame().GetLoadingScreen())
        __loadingScreen.displayText.SetColor(Vec4.new(1.0, 1.0, 1.0, 0.0))
        __loadingScreen.displayText.SetText("I'm not certain what's happening to me, why shouldn't I join them?")

        __timer = 0
    }

    static Shutdown(engine) {
        System.print("Exit loading screen")
    }

    static Update(engine, dt) {

        var textColor = Vec4.new(1.0, 1.0, 1.0, __timer / 2000.0)
        var text = __loadingScreen.displayText
        var color = text.SetColor(textColor)

        if(__timer > 2000.0) {
            engine.TransitionToScript("game/game.wren")
        }

        __timer = __timer + dt
    }

}