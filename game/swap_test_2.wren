import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "gameplay/movement.wren" for PlayerMovement

class Main {

    static Start(engine) {
        var dEntity = engine.GetECS().NewEntity()
        var transform = dEntity.AddTransformComponent()
        var name = dEntity.AddNameComponent()
        name.name = "Directional Light"

        var dLight = dEntity.AddDirectionalLightComponent()
        dLight.color = Vec3.new(5.0, 0.0, 5.0)
        dLight.planes = Vec2.new(0.1, 1000.0)
        dLight.orthographicSize = 75.0

        transform.translation = Vec3.new(-105.0, 68.0, 168.0)
        transform.rotation = Quat.new(-0.29, 0.06, -0.93, -0.19)

        var spheres = engine.LoadModel("assets/models/DamagedHelmet.glb")[0]

        __camera = engine.GetECS().GetEntityByName("Camera")
        __playerMovement = PlayerMovement.new(false,0.0)
        __playerController = engine.GetGame().CreatePlayerController(engine.GetPhysics(),engine.GetECS(),Vec3.new(-18.3, 30.3, 193.8),1.7,0.5)
        __playerController.AddCheatsComponent()



        //System.print("Sphere name: " + sName.name)

        


    }

    static Shutdown(engine) {

    }

    static Update(engine, dt) {

        __playerMovement.Update(engine,dt,__playerController, __camera)

    }
}