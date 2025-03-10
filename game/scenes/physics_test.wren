import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random

class Main {

    static Start(engine) {

        var ecs = engine.GetECS()

        __collider1 = ecs.NewEntity()
        __collider2 = ecs.NewEntity()

        var circleShape = ShapeFactory.MakeSphereShape(1.0)
        
        __collider1.AddTransformComponent().translation = Vec3.new(0.0, 0.0, 0.0)
        __collider2.AddTransformComponent().translation = Vec3.new(5.0, 0.0, 0.0)

        var rb1 = Rigidbody.new(engine.GetPhysics(), circleShape, false, false)
        var rb2 = Rigidbody.new(engine.GetPhysics(), circleShape, false, false)

        __collider1.AddRigidbodyComponent(rb1)
        __collider1.AddRigidbodyComponent(rb2)
    }

    static Shutdown(engine) {

        engine.GetECS().DestroyAllEntities()
    }

    static Update(engine, dt) {
 
    }
}