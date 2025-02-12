import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

class Weapons {
    static pistol {0}
    static shotgun {1}
    static knife {2}
}

class WeaponBase {
    construct new(engine) {
        _damage
        _range
        _attackSpeed
        _mesh
        _shootAnim
        _equipAnim
        _reloadAnim
        _shootSFX
        _equipSFX
        _reloadSFX
        _cooldown = 0
    }

    reload (engine) {
        System.print("Base reload")
    }

    attack(engine) {
        System.print("Base attack")
    }

    equip (engine) {
        System.print("Base equip")    
    }

    // damage {_damage}
    // range {_range}
    // attackSpeed {_attackSpeed}
    // mesh {_mesh}
    // shootAnim {_shootAnim}
    // equipAnim {_equipAnim}
    // reloadAnim {_reloadAnim}
    // shootSFX {_shootSFX}
    // equipSFX {_equipSFX}
    // reloadSFX {_reloadSFX}

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}

}

class Pistol is WeaponBase {
    construct new(engine) {
        _damage = 20
        _range = 50
        _attackSpeed = 0.2 * 1000
        _maxAmmo = 6
        _ammo = _maxAmmo
        _cooldown = 0


        // animations

        // SFX

        // mesh

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)
    }

    reload (engine) {
        System.print("Pistol reload")
    }

    attack(engine, deltaTime) {

        if (_cooldown <= 0) {
            System.print("Pistol shoot")

            player = engine.GetECS().GetEntityByName("Camera")
            gun = engine.GetECS().GetEntityByName("AnimatedRifle")

    // Play shooting audio
            var shootingInstance = engine.GetAudio().PlayEventOnce("event:/Weapons/Machine Gun")
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)

    // Spawn particles
            var playerTransform = player.GetTransformComponent()
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

    // Play shooting animation
            var gunAnimations = gun.GetAnimationControlComponent()
            if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
                gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
            }
            if(engine.GetInput().GetDigitalAction("Shoot").IsPressed()) {
                if(gunAnimations.AnimationFinished()) {
                    gunAnimations.Play("Armature|Armature|Shoot", 2.0, false)
                }
            }
            
            _cooldown = _attackSpeed
        } 
    }

    equip (engine) {
        System.print("Pistol equip")
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}
}


class Shotgun is WeaponBase {
    construct new(engine) {
        _damage = 20
        _range = 50
        _attackSpeed = 0.2 * 1000
        _maxAmmo = 6
        _ammo = _maxAmmo
        _cooldown = 0
    }
    
    reload (engine) {
        System.print("Shotgun reload")
    }

    attack(engine, deltaTime, player) {
        System.print("Shotgun shoot")
    }

    equip (engine) {
        System.print("Shotgun equip")    
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}
}

class Knife is WeaponBase {
    construct new(engine) {
        _damage = 20
        _range = 50
        _attackSpeed = 0.2 * 1000
        _maxAmmo = 6
        _ammo = _maxAmmo
        _cooldown = 0
    }

    reload (engine) {
        System.print("Knife reload")
    }

    attack(engine, deltaTime, player) {
        System.print("Knife stab")
    }

    equip (engine) {
        System.print("Knife equip")   
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}
}