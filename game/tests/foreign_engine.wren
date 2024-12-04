import "engine_api.wren" for Engine, TimeModule

class Test {
    static test(engine) {

        var time_module = engine.GetTime()
        var deltatime = time_module.GetDeltatime()

        System.print(deltatime)
    }
}