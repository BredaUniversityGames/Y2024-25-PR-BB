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
        _reloadTimer = 0
        _reloadSpeed = 1.2 * 1000
        
        _attackSFX = "event:/Weapons/Pistol"
        _reloadSFX = ""
        _equipSFX = ""
        
        _attackAnim = "Armature|Armature|Shoot"
        _reloadAnim = "Armature|Armature|Reload"
        _equipAnim = "" 

        _mesh = ""
    }

    reload (engine) {
        System.print("Pistol reload")

        var gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
            gunAnimations.Play(_reloadAnim, 1.0, false)
        }

        _reloadTimer = _reloadSpeed
        _ammo = _maxAmmo
    }

    attack(engine, deltaTime) {

        if (_cooldown <= 0 && _ammo > 0 && _reloadTimer <= 0) {
            _ammo = _ammo - 1
            System.print("Pistol shoot")

            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName("AnimatedRifle")

            // Play shooting audio
            var eventInstance = engine.GetAudio().PlayEventOnce(_attackSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)

            // Spawn particles
            var playerTransform = player.GetTransformComponent()
            var direction = Math.ToVector(playerTransform.rotation)
            var start = playerTransform.translation + direction * Vec3.new(2.0, 2.0, 2.0)
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, _range)
            var end = start + direction * _rangeVector 

            if (!rayHitInfo.isEmpty) {
                end = rayHitInfo[0].position
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = end
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 300.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eImpact(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), rayHitInfo[0].normal * Vec3.new(5, 5, 5))
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
            gunAnimations.Play(_attackAnim, 2.0, false)
            
            _cooldown = _attackSpeed
        } 
    }

    equip (engine) {
        System.print("Pistol equip")
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}

    reloadTimer {_reloadTimer}
    reloadTimer=(value) {_reloadTimer = value}

    ammo {_ammo}
    ammo=(value) {_ammo = value}

    maxAmmo {_maxAmmo}
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
        _reloadTimer = 0
        _reloadSpeed = 0.4 * 1000
        _spread = [Vec2.new(0, 0), Vec2.new(-1, 1), Vec2.new(0, 1), Vec2.new(1, 1), Vec2.new(0, 2), Vec2.new(-1, -1), Vec2.new(0, -1), Vec2.new(1, -1), Vec2.new(0, -2)]

        _attackSFX = "event:/Weapons/Explosion"
        _reloadSFX = ""
        _equipSFX = ""
        
        _attackAnim = "Armature|Armature|Shoot"
        _reloadAnim = "Armature|Armature|Reload"
        _equipAnim = "" 

        _mesh = ""
    }
    
    reload (engine) {
        System.print("Shotgun reload")

        var gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
            gunAnimations.Play(_reloadAnim, 1.0, false)
        }

        _reloadTimer = _reloadSpeed
        _ammo = _maxAmmo
    }

    attack(engine, deltaTime) {   
        if (_cooldown <= 0 && _ammo > 0 && _reloadTimer <= 0) {
            _ammo = _ammo - 1
            System.print("Shotgun shoot")

            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName("AnimatedRifle")

            // Play shooting audio
            var shootingInstance = engine.GetAudio().PlayEventOnce(_attackSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)
            
            // Spawn particles
            var playerTransform = player.GetTransformComponent()
            var direction = Math.ToVector(playerTransform.rotation)
            var start = playerTransform.translation + direction * Vec3.new(2.0, 2.0, 2.0)
            
            var i = 0
            while (i < _raysPerShot) {

                var newDirection = Math.RotateForwardVector(direction, Vec2.new(_spread[i].x * 3, _spread[i].y * 3), playerTransform.rotation.mul(Vec3.new(0, 1, 0)))                

                var rayHitInfo = engine.GetPhysics().ShootRay(start, newDirection, _range)
                
                var end = start + newDirection * _rangeVector
                
                if (!rayHitInfo.isEmpty) {
                    end = rayHitInfo[0].position
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
            gunAnimations.Play(_attackAnim, 2.0, false)            
            _cooldown = _attackSpeed
        }
    }

    equip (engine) {
        System.print("Shotgun equip")    
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}

    reloadTimer {_reloadTimer}
    reloadTimer=(value) {_reloadTimer = value}

    ammo {_ammo}
    ammo=(value) {_ammo = value}

    maxAmmo {_maxAmmo}
}

class Knife {
    construct new(engine) {
        _damage = 100
        _range = 3
        _attackSpeed = 0.2 * 1000
        _cooldown = 0
        _reloadTimer = 0
        _reloadSpeed = 0
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

    reloadTimer {_reloadTimer}
    reloadTimer=(value) {_reloadTimer = value}

    ammo {_ammo}
    ammo=(value) {_ammo = value}

    maxAmmo {_maxAmmo}
}