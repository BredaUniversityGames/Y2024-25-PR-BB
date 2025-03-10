import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "gameplay/movement.wren" for PlayerMovement

class Main {
    static Start(engine) {

        // Directional Light
        __directionalLight = engine.GetECS().NewEntity()
        __directionalLight.AddNameComponent().name = "Directional Light"

        var comp = __directionalLight.AddDirectionalLightComponent()
        comp.color = Vec3.new(0.0, 0.0, 5.0)
        comp.planes = Vec2.new(0.1, 1000.0)
        comp.orthographicSize = 75.0

        var transform = __directionalLight.AddTransformComponent()
        transform.translation = Vec3.new(-105.0, 68.0, 168.0)
        transform.rotation = Quat.new(-0.29, 0.06, -0.93, -0.19)

        // Models
        engine.LoadModel("assets/models/MetalRoughSpheres.glb")
    }

    static Shutdown(engine) {
        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {

        var input = engine.GetInput()

        if (engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            engine.TransitionToScript("game/scenes/swap_test_2.wren")
            return
        }
    }
}