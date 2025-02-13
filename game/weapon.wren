import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

class Weapons {
    static pistol {0}
    static shotgun {1}
    static knife {2}
}

class Pistol {
    construct new(engine) {
        _damage = 20
        _range = 50
        _rangeVector = Vec3.new(_range, _range, _range)
        _attackSpeed = 0.2 * 1000
        _maxAmmo = 6
        _ammo = _maxAmmo
        _cooldown = 0

        // animations

        // SFX

        // mesh
    }

    reload (engine) {
        System.print("Pistol reload")

        var gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
            gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
        }

        _ammo = _maxAmmo
    }

    attack(engine, deltaTime) {

        if (_cooldown <= 0 && _ammo > 0) {
            _ammo = _ammo - 1
            System.print("Pistol shoot")

            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName("AnimatedRifle")

            // Play shooting audio
            var shootingInstance = engine.GetAudio().PlayEventOnce("event:/Weapons/Machine Gun")
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)

            // Spawn particles
            var playerTransform = player.GetTransformComponent()
            var direction = Math.ToVector(playerTransform.rotation)
            var start = playerTransform.translation + direction * Vec3.new(2.0, 2.0, 2.0)
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, _range)
            var end = rayHitInfo.position

            if (rayHitInfo.hasHit) {
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = end
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 300.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eImpact(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 5.0, 0.0))
            } else {
                end = start + direction * _rangeVector
            }

            var length = (end - start).length()
            var i = 5.0
            while (i < length) {
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = Math.Mix(start, end, i / length)
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 200.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eRay(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), direction * Vec3.new(10, 10, 10))
                i = i + 5.0
            }

            // Play shooting animation
            var gunAnimations = gun.GetAnimationControlComponent()
            gunAnimations.Play("Armature|Armature|Shoot", 2.0, false)
            
            _cooldown = _attackSpeed
        } 
    }

    equip (engine) {
        System.print("Pistol equip")
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}
}


class Shotgun {
    construct new(engine) {
        _damage = 8
        _damageDropoff = 0.5
        _raysPerShot = 9
        _range = 23
        _rangeVector = Vec3.new(_range, _range, _range)
        _attackSpeed = 0.8 * 1000
        _maxAmmo = 2
        _ammo = _maxAmmo
        _cooldown = 0
        _spread = [Vec2.new(0, 0), Vec2.new(-1, 1), Vec2.new(0, 1), Vec2.new(1, 1), Vec2.new(0, 2), Vec2.new(-1, -1), Vec2.new(0, -1), Vec2.new(1, -1), Vec2.new(0, -2)]
    }
    
    reload (engine) {
        System.print("Shotgun reload")

        var gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
            gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
        }

        _ammo = _maxAmmo
    }

    attack(engine, deltaTime) {   
        if (_cooldown <= 0 && _ammo > 0) {
            _ammo = _ammo - 1
            System.print("Shotgun shoot")

            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName("AnimatedRifle")

            // Play shooting audio
            var shootingInstance = engine.GetAudio().PlayEventOnce("event:/Weapons/Machine Gun")
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)
            
            // Spawn particles
            var playerTransform = player.GetTransformComponent()
            var direction = Math.ToVector(playerTransform.rotation)
            var start = playerTransform.translation + direction * Vec3.new(2.0, 2.0, 2.0)
            
            var i = 0
            while (i < _raysPerShot) {

                var newDirection = Math.RotateForwardVector(direction, Vec2.new(_spread[i].x * 10, _spread[i].y * 10), playerTransform.rotation.mul(Vec3.new(0, 1, 0)))                

                var rayHitInfo = engine.GetPhysics().ShootRay(start, newDirection, _range)
                
                var end = start + newDirection * _rangeVector
                
                if (rayHitInfo.hasHit) {
                    end = rayHitInfo.position
                }

                var length = (end - start).length()
                var j = 1.0
                while (j < length) {
                    var entity = engine.GetECS().NewEntity()
                    var transform = entity.AddTransformComponent()
                    transform.translation = Math.Mix(start, end, j / length)
                    var lifetime = entity.AddLifetimeComponent()
                    lifetime.lifetime = 200.0
                    var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                    engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eRay(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), newDirection * Vec3.new(10, 10, 10))
                    j = j + 1.0
                }

                i = i + 1
            }
            // Play shooting animation
            var gunAnimations = gun.GetAnimationControlComponent()
            gunAnimations.Play("Armature|Armature|Shoot", 2.0, false)            
            _cooldown = _attackSpeed
        }
    }

    equip (engine) {
        System.print("Shotgun equip")    
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}
}

class Knife {
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

    attack(engine, deltaTime) {
        System.print("Knife stab")
    }

    equip (engine) {
        System.print("Knife equip")   
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}
}