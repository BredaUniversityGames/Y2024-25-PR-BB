import "engine_api.wren" for Engine, Vec4, Random, Input

class Main {

    static Start(engine) {
        System.print("Start loading screen")

        engine.GetInput().SetActiveActionSet("UserInterface")

        var textOptions = [
            "\"Time to make easy money.\"",
            "Poverty always drives someone to the end of the world.",
            "Ever since the plague,\nthe church has stood silent watching the dead rise.",
            "\"Let's hope the abyss doesn't stare back this time.\"",
            "\"Digging up the dead was never quiet work.\"",
            "Don't forget your offerings!",
            "\"Coins and souls are all I need.\"",
            "Every bullet fired is a prayer to a god long dead.",
            "The plague was only the beginning,\nnow the dead claw for more than life.",
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