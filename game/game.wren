import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, TransformComponent

class Main {

    static Start(engine) {
        __counter = 0
        __frameTimer = 0
        __player = engine.GetECS().GetEntityByName("Camera")

        if (__player) {
            System.print("Player is online!")

            var t = __player.GetTransformComponent()
            t.translation = Vec3.new(4.5, 35.0, 285.0)
        }
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