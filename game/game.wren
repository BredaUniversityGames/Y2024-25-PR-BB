import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

class Base {
    construct new(engine) {
        _width = Vec3.new(1, 1, 1)
        _engine = engine
    }

    shoot() {
        System.print("base")
    }

    width { _width }

    width=(value) { _width = value }
}

class Derived is Base {

    construct new() {
        _width = Vec3.new(2, 2, 2)
    }

    shoot() {
        System.print("derived")
    }
} 

class Main {

    static Start(engine) {
        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")

        __base = Base.new(engine)
        __derived = Derived.new()

        __armory = [__base, __derived]

        for (thing in __armory) {
            thing.shoot()
        }

        __activeWeapon = __armory[0]

        __activeWeapon.shoot()


        __counter = 0
        __frameTimer = 0
        __timer = 0
        __player = engine.GetECS().GetEntityByName("Camera")
        __gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = __gun.GetAnimationControlComponent()
        gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
        gunAnimations.Stop()

        if (__player) {
            System.print("Player is online!")

            var playerTransform = __player.GetTransformComponent()
            playerTransform.translation = Vec3.new(4.5, 35.0, 285.0)

            __player.AddAudioEmitterComponent()

            //var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity()
            //engine.GetParticles().SpawnEmitter(__player, EmitterPresetID.eTest(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(5.0, -1.0, -5.0))

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))
        }

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)
    }

    static Update(engine, dt) {
        __counter = __counter + 1

        var deltaTime = engine.GetTime().GetDeltatime()
        __frameTimer = __frameTimer + dt
        __timer = __timer + dt

        if (__frameTimer > 1000.0) {
            System.print("%(__counter) Frames per second")
            __frameTimer = __frameTimer - 1000.0
            __counter = 0
        }

        if (engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            var shootingInstance = engine.GetAudio().PlayEventOnce("event:/Weapons/Machine Gun")
            var audioEmitter = __player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)

            System.print("Playing is shooting")
        }

        if (engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            var playerTransform = __player.GetTransformComponent()
            var direction = Math.ToVector(playerTransform.rotation)
            var start = playerTransform.translation + direction * Vec3.new(2.0, 2.0, 2.0)
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, __rayDistance)
            var end = rayHitInfo.position

            if (rayHitInfo.hasHit) {
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = end
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 1000.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive()
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eTest(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
            } else {
                end = start + direction * __rayDistanceVector
            }

            var length = (end - start).length()
            var i = 5.0
            while (i < length) {
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = Math.Mix(start, end, i / length)
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 1000.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive()
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eTest(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
                i = i + 5.0
            }
        }

        if (engine.GetInput().GetDigitalAction("Jump").IsPressed()) {
            System.print("Player Jumped!")

        }

        var gunAnimations = __gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
            gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
        }
        if(engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            if(gunAnimations.AnimationFinished()) {
                gunAnimations.Play("Armature|Armature|Shoot", 2.0, false)
            }
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
