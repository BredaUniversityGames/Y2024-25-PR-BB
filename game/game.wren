import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "weapon.wren" for Pistol, Shotgun, Knife, Weapons
import "gameplay/movement.wren" for PlayerMovement

class Main {

    static Start(engine) {

        // Set navigational mesh
        engine.GetPathfinding().SetNavigationMesh("assets/models/NavmeshTest/LevelNavmeshTest.glb")

        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")

        __playerMovement = PlayerMovement.new(false,0.0)
        __counter = 0
        __frameTimer = 0
        __groundedTimer = 0
        __hasDashed = false
        __timer = 0
        __camera = engine.GetECS().GetEntityByName("Camera")
        __playerController = engine.GetGame().CreatePlayerController(engine.GetPhysics(), engine.GetECS(), Vec3.new(-18.3, 30.3, 193.8), 1.7, 0.5)
        __gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = __gun.GetAnimationControlComponent()
        gunAnimations.Play("Reload", 1.0, false)
        gunAnimations.Stop()

        var demons = engine.GetECS().GetEntitiesByName("Demon")
        for(demon in demons) {
            var demonAnimations = demon.GetAnimationControlComponent()
            demonAnimations.Play("Armature.001|A_Run_F_Masc|BaseLayer", 1.0, true)
        }

        if (__camera) {
            System.print("Player is online!")

            var playerTransform = __camera.GetTransformComponent()
            //playerTransform.translation = Vec3.new(4.5, 35.0, 285.0)

            __camera.AddAudioEmitterComponent()
            __playerController.AddCheatsComponent()

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))
        }

        __armory = [Pistol.new(engine), Shotgun.new(engine), Knife.new(engine)]

        __activeWeapon = __armory[Weapons.shotgun]
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

        __ultimateCharge = 0
        __ultimateActive = false

        var enemyEntity = engine.LoadModel("assets/models/demon.glb")[0]
        var enemyTransform = enemyEntity.GetTransformComponent()
        enemyTransform.scale = Vec3.new(0.03, 0.03, 0.03)
        enemyTransform.translation = Vec3.new(4.5, 35.0, 285.0)
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

        if (__ultimateActive) {
            __ultimateCharge = __ultimateCharge - dt
            if (__ultimateCharge == 0) {
                __activeWeapon = __armory[Weapons.pistol]
                __activeWeapon.equip()
                __ultimateActive = false
            }
        } else {
            __ultimateCharge = __ultimateCharge + dt
        }

        if (__frameTimer > 1000.0) {
            __frameTimer = __frameTimer - 1000.0
            __counter = 0
        }

        if(engine.GetInput().DebugGetKey(Keycode.eN())){
           cheats.noClip = !cheats.noClip
        }

        __playerMovement.Update(engine, dt, __playerController, __camera)

        for (weapon in __armory) {
            weapon.cooldown = Math.Max(weapon.cooldown - dt, 0)

            weapon.reloadTimer = Math.Max(weapon.reloadTimer - dt, 0)
            if (weapon != __activeWeapon) {
                if (weapon.reloadTimer <= 0) {
                    weapon.ammo = weapon.maxAmmo
                }
            }
        }

        if (engine.GetInput().GetDigitalAction("Reload").IsHeld()) {
            __activeWeapon.reload(engine)
        }

        if (engine.GetInput().GetDigitalAction("Shoot").IsHeld()) {
            __activeWeapon.attack(engine, dt)
        }

        if (engine.GetInput().GetDigitalAction("Ultimate").IsPressed()) {
            if (__ultimateCharge == 1000) {
                __activeWeapon = __armory[Weapons.shotgun]
                __activeWeapon.equip()
            }
        }

        if (engine.GetInput().DebugGetKey(Keycode.eV())) {
            __armory[Weapons.knife].attack(engine, dt)
        }

        if (engine.GetInput().DebugGetKey(Keycode.e1())) {
            __activeWeapon = __armory[Weapons.pistol]
            __activeWeapon.equip(engine)
        }

        if (engine.GetInput().DebugGetKey(Keycode.e2())) {
            __activeWeapon = __armory[Weapons.shotgun]
            __activeWeapon.equip(engine)
        }


        var path = engine.GetPathfinding().FindPath(Vec3.new(-42.8, 19.3, 267.6), Vec3.new(-16.0, 29.0, 195.1))
    }
}