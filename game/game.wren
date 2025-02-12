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

            //var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity()
            //engine.GetParticles().SpawnEmitter(__player, EmitterPresetID.eTest(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(5.0, -1.0, -5.0))

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))
        }

        __armory = [Pistol.new(engine), Shotgun.new(engine), Knife.new(engine)]

        __activeWeapon = __armory[Weapons.pistol]
        __activeWeapon.equip(engine)        
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

        if (engine.GetInput().GetDigitalAction("Jump").IsPressed()) {
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
