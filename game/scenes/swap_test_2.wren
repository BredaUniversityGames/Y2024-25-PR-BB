import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "gameplay/movement.wren" for PlayerMovement

class Main {

    static Start(engine) {
       
    }

    static Shutdown(engine) {
        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {

        if (engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            engine.TransitionToScript("game/scenes/swap_test.wren")
            return
        }

    }
}