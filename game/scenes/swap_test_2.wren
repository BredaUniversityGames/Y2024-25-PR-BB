import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "gameplay/movement.wren" for PlayerMovement

class Main {

    static Start(engine) {
       // Camera
       __camera = engine.GetECS().NewEntity()

       var cameraProperties = __camera.AddCameraComponent()
       cameraProperties.fov = 45.0
       cameraProperties.nearPlane = 0.5
       cameraProperties.farPlane = 600.0
       cameraProperties.reversedZ = true

        __camera.AddTransformComponent()
       __camera.AddAudioEmitterComponent()
       __camera.AddNameComponent().name = "Camera"
       __camera.AddAudioListenerTag()

       var entity = engine.LoadModel("assets/models/Clown.glb")
       entity.GetTransformComponent().translation = Vec3.new(0.0, -100.0, -300.0)
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