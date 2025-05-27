import "engine_api.wren" for Engine, Vec4, Random, Input

class Main {

    static Start(engine) {
        System.print("Start loading screen")

        engine.GetInput().SetActiveActionSet("UserInterface")

        var textOptions = [
            "\"I'm not certain what's happening to me\nWhy shouldn't I join them?\nWhy must I remain here?\"",
            "\"I wish I could go back\nMake different choices\"",
            "Poverty drives a man to discoveries he wished he hadn't made"
        ]

        var index = Random.RandomIndex(0, textOptions.count)

        __loadingScreen = engine.GetGame().GetLoadingScreen()
        engine.GetGame().SetUIMenu(engine.GetGame().GetLoadingScreen())
        __loadingScreen.SetDisplayTextColor(Vec4.new(1.0, 1.0, 1.0, 0.0))
        __loadingScreen.SetDisplayText(textOptions[index])
        __loadingScreen.HideContinuePrompt()

        __timer = 0
        __loadLevel = false
    }

    static Shutdown(engine) {
        System.print("Exit loading screen")
    }

    static Update(engine, dt) {

        var textColor = Vec4.new(1.0, 1.0, 1.0, __timer / 2000.0)
        var text = __loadingScreen.SetDisplayTextColor(textColor)

        if(__loadLevel) {
            engine.TransitionToScript("game/game.wren")
        }

        if(__timer > 2000.0) {
            engine.GetGame().GetLoadingScreen().ShowContinuePrompt()
            if(engine.GetInput().GetDigitalAction("Interact").IsPressed()) {
                __loadLevel = true
                __loadingScreen.SetDisplayText("Loading")
                __loadingScreen.HideContinuePrompt()
            }
        }

        __timer = __timer + dt
    }

}