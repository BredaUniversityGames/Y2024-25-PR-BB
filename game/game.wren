import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "gameplay/movement.wren" for PlayerMovement, EnemyMovement

class Main {

    static Start(engine) {

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/NavmeshTest/LevelNavmeshTest.glb")

        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")

        __enemyMovement = EnemyMovement.new(engine)
        __playerMovement = PlayerMovement.new(false,0.0)
        __counter = 0
        __frameTimer = 0
        __groundedTimer = 0
        __hasDashed = false
        __timer = 0
        __camera = engine.GetECS().GetEntityByName("Camera")
        __playerController = engine.GetGame().CreatePlayerController(engine.GetPhysics(),engine.GetECS(),Vec3.new(-18.3, 30.3, 193.8),1.7,0.5)
        __gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = __gun.GetAnimationControlComponent()
        gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
        gunAnimations.Stop()

        if (__camera) {
            System.print("Player is online!")

            var playerTransform = __camera.GetTransformComponent()
            playerTransform.translation = Vec3.new(4.5, 35.0, 285.0)

            __camera.AddAudioEmitterComponent()
            __playerController.AddCheatsComponent()

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))
        }

        // Inside cathedral: pentagram scene
        {   // Fire emitter 1
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-18.3, 30.3, 193.8), Vec3.new(0.0, 0.0, 0.0))
        }

        {   // Fire emitter 2
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-18.3, 30.3, 190.4), Vec3.new(0.0, 0.0, 0.0))
        }

        {   // Fire emitter 3
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-14.9, 30.3, 190.4), Vec3.new(0.0, 0.0, 0.0))
        }

        {   // Fire emitter 4
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eFlame(), emitterFlags, Vec3.new(-14.9, 30.3, 193.8), Vec3.new(0.0, 0.0, 0.0))
        }

        {   // Dust emitter
            var emitter = engine.GetECS().NewEntity()
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomPosition() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(emitter, EmitterPresetID.eDust(), emitterFlags, Vec3.new(-17.0, 34.0, 196.0), Vec3.new(1.0, 0.0, 0.0))
        }

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)
    }

    static Shutdown(engine) {
        System.print("Shutdown script!")
    }

    static Update(engine, dt) {
        __counter = __counter + 1
        var cheats = __playerController.GetCheatsComponent()
        var deltaTime = engine.GetTime().GetDeltatime()
        __frameTimer = __frameTimer + dt
        __timer = __timer + dt

        if (__frameTimer > 1000.0) {
            //System.print("%(__counter) Frames per second")
            __frameTimer = __frameTimer - 1000.0
            __counter = 0
        }

        if (engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            var shootingInstance = engine.GetAudio().PlayEventOnce("event:/Weapons/Machine Gun")
            var audioEmitter = __camera.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)

            System.print("Playing is shooting")
        }

         if (engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
            var playerTransform = __camera.GetTransformComponent()
            var direction = Math.ToVector(playerTransform.rotation)
            var start = playerTransform.translation + direction * Vec3.new(2.0, 2.0, 2.0)
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, __rayDistance)
            var end = start + direction * __rayDistanceVector
            if(!rayHitInfo.isEmpty) {
                end = rayHitInfo[0].position
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = end
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 300.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity()
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eImpact(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 5.0, 0.0))
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
                lifetime.lifetime = 200.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity()
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eRay(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), direction * Vec3.new(10, 10, 10))
                i = i + 5.0
            }
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


        if(engine.GetInput().DebugGetKey(Keycode.eN())){
           cheats.noClip = !cheats.noClip
        }

        __playerMovement.Update(engine,dt,__playerController, __camera)
        __enemyMovement.Update(engine)

        var path = engine.GetPathfinding().FindPath(Vec3.new(-42.8, 19.3, 267.6), Vec3.new(-16.0, 29.0, 195.1))
    }
}