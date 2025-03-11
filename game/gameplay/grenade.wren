import "engine_api.wren" for Engine, TimeModule, ECS, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID, Random

class Grenade {
    construct new(engine, player) {
        _maxDelay = 2
        _damage = 100
        _explosionRadius = 5
        _radius = 0.5
        _cameraShakeIntensity = 1
        _throwStrength = 50
        _SFX = "event:/Weapons/Explosion"
        _delay = _maxDelay

        _entity = engine.GetECS().NewEntity()
        _entity.AddTransformComponent().translation = player.GetTransformComponent().GetWorldTranslation()
        _entity.AddNameComponent().name = "Grenade"

        // Create collider
        var shape = ShapeFactory.MakeSphereShape(0.5)
        var rb = Rigidbody.new(engine.GetPhysics(), shape, true, true)
        var rbComponent = _entity.AddRigidbodyComponent(rb)

        var velocity = Math.ToVector(player.GetTransformComponent().GetWorldRotation()).normalize().mulScalar(_throwStrength)

        rbComponent.SetVelocity(velocity)
    }

    delay {_delay}
    delay=(value) {_delay = value}

    cameraShakeIntensity {_cameraShakeIntensity}

    damage {_damage}



    Explode(engine) {
        var translation = _entity.GetTransformComponent().GetWorldTranslation()

        var particle = engine.GetECS().NewEntity()
        particle.AddTransformComponent().translation = translation

        var eventInstance = engine.GetAudio().PlayEventOnce(_SFX)
        particle.AddAudioEmitterComponent().AddEvent(eventInstance)

        var lifetime = particle.AddLifetimeComponent()
        lifetime.lifetime = 200.0
        
        var emitterFlags = SpawnEmitterFlagBits.eIsActive()
        engine.GetParticles().SpawnEmitter(particle, EmitterPresetID.eImpact(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0, 0, 0))    
    }

    Destroy(engine) {
        engine.GetECS().DestroyEntity(_entity)
    }

}