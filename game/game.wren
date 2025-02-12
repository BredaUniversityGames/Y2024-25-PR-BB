import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "weapon.wren" for WeaponBase, Pistol, Shotgun, Knife, Weapons

class Main {

    static Start(engine) {
        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")

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

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))
        }

        __armory = [Pistol.new(engine), Shotgun.new(engine), Knife.new(engine)]

        __activeWeapon = __armory[Weapons.pistol]
        __activeWeapon.equip(engine)
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

        for (weapon in __armory) {
            weapon.cooldown = Math.Max(weapon.cooldown - dt, 0)
        }

        if (engine.GetInput().GetDigitalAction("Shoot").IsHeld()) {
            __activeWeapon.attack(engine, dt)
            //System.print("Playing is shooting")
        }

        if (engine.GetInput().GetDigitalAction("Reload").IsHeld()) {
            __activeWeapon.reload(engine)
            //System.print("Playing is shooting")
        }


        if (engine.GetInput().GetDigitalAction("Jump").IsPressed()) {
            //System.print("Player Jumped!")

        }

        var movement = engine.GetInput().GetAnalogAction("Move")

        if (movement.length() > 0) {
            //System.print("Player is moving")
        }

        var key = Keycode.eA()
        if (engine.GetInput().DebugGetKey(key)) {
            //System.print("[Debug] Player pressed A!")
        }
    }
}