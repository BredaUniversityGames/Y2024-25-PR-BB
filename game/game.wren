import "engine_api.wren" for Engine, TimeModule

class Main {

    static Start(engine) {
        __counter = 0
        __frameTimer = 0
    }

    static Update(engine, dt) {
        __counter = __counter + 1

        var deltatime = engine.GetTime().GetDeltatime()
        __frameTimer = __frameTimer + dt

        if (__frameTimer > 1000.0) {
            System.print("%(__counter) Frames per second")
            __frameTimer = __frameTimer - 1000.0
            __counter = 0
        }
    }
}