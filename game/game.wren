import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, TransformComponent, Input, Keycode

class Main {

    static Start(engine) {
        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")
        
        __counter = 0
        __frameTimer = 0
        __shootingInstance = 0
        __player = engine.GetECS().GetEntityByName("Camera")

        if (__player) {
            System.print("Player is online!")

            __player.AddAudioEmitterComponent()

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

        __player = engine.GetECS().GetEntityByName("Camera")
        var audioEmitterComponent = __player.GetAudioEmitterComponent()
        
        if (engine.GetInput().GetDigitalAction("Shoot")) {
            __shootingInstance = engine.GetAudio().PlayEventLoop("event:/Weapons/Machine Gun")
            audioEmitterComponent.AddEvent(__shootingInstance)
            System.print("Playing is shooting")
        }

        if (engine.GetInput().GetDigitalAction("ReleaseTrigger")) {
            engine.GetAudio().StopEvent(__shootingInstance)
            System.print("Player has stopped shooting")
        }

        if (engine.GetInput().GetDigitalAction("Jump")) {
            System.print("Player Jumped!")
        }

        var movement = engine.GetInput().GetAnalogAction("Move")

        if (movement.length() > 0) {
            System.print("Player is moving")
        }

        var key = Keycode.eA()
        if (engine.GetInput().DebugGetKey(key)) {
            System.print("[Debug] Player pressed A!")
        }
    }
}