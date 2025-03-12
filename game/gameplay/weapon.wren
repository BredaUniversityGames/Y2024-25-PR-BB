import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID
import "camera.wren" for CameraVariables

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

        _cameraShakeIntensity = 0.3
        
        _attackSFX = "event:/Weapons/Pistol"
        _reloadSFX = ""
        _equipSFX = ""
        
        _attackAnim = "Shoot"
        _reloadAnim = "Reload"
        _equipAnim = "" 

        _mesh = ""
    }

    reload (engine) {
        System.print("Pistol reload")

        var gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
            gunAnimations.Play(_reloadAnim, 0.0, 1.0, false)
        }

        _reloadTimer = _reloadSpeed
        _ammo = _maxAmmo
    }

    attack(engine, deltaTime, cameraVariables) {

        if (_cooldown <= 0 && _ammo > 0 && _reloadTimer <= 0) {
            _ammo = _ammo - 1
            System.print("Pistol shoot")

            // Shake the camera

            cameraVariables.shakeIntensity = _cameraShakeIntensity            

            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName("AnimatedRifle")

            // Play shooting audio
            var eventInstance = engine.GetAudio().PlayEventOnce(_attackSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)

            // Spawn particles
            var playerTransform = player.GetTransformComponent()
            var translation = playerTransform.GetWorldTranslation()
            var rotation = playerTransform.GetWorldRotation()
            var forward = Math.ToVector(rotation)
            var up = rotation.mulVec3(Vec3.new(0, 1, 0))
            var right = Math.Cross(forward, up)
            var start = translation + forward * Vec3.new(1, 1, 1) - right * Vec3.new(0.09, 0.09, 0.09) - up * Vec3.new(0.12, 0.12, 0.12)
            var end = translation + forward * _rangeVector
            var direction = (end - start).normalize()
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, _range)

            if (!rayHitInfo.isEmpty) {
                end = rayHitInfo[0].position
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = end
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 300.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eImpact(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), rayHitInfo[0].normal)
            }

            var length = (end - start).length()
            var i = 1.0
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
            gunAnimations.Play(_attackAnim, 0.0, 2.0, false)
            
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
        _cameraShakeIntensity = 0.5

        _attackSFX = "event:/Weapons/Explosion"
        _reloadSFX = ""
        _equipSFX = ""
        
        _attackAnim = "Shoot"
        _reloadAnim = "Reload"
        _equipAnim = "" 

        _mesh = ""
    }
    
    reload (engine) {
        System.print("Shotgun reload")

        var gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() && gunAnimations.AnimationFinished()) {
            gunAnimations.Play(_reloadAnim, 0.0, 1.0, false)
        }

        _reloadTimer = _reloadSpeed
        _ammo = _maxAmmo
    }

    attack(engine, deltaTime, cameraVariables) {   
        if (_cooldown <= 0 && _ammo > 0 && _reloadTimer <= 0) {
            _ammo = _ammo - 1
            System.print("Shotgun shoot")

            cameraVariables.shakeIntensity = _cameraShakeIntensity

            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName("AnimatedRifle")

            // Play shooting audio
            var shootingInstance = engine.GetAudio().PlayEventOnce(_attackSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)
            
            // Spawn particles
            var playerTransform = player.GetTransformComponent()
            var translation = playerTransform.GetWorldTranslation()
            var rotation = playerTransform.GetWorldRotation()
            var forward = Math.ToVector(rotation)
            var up = rotation.mulVec3(Vec3.new(0, 1, 0))
            var right = Math.Cross(forward, up)
            var start = translation + forward * Vec3.new(1, 1, 1) - right * Vec3.new(0.09, 0.09, 0.09) - up * Vec3.new(0.12, 0.12, 0.12)
            var end = translation + forward * _rangeVector
            var direction = (end - start).normalize()
            
            var i = 0
            while (i < _raysPerShot) {
                var newDirection = Math.RotateForwardVector(direction, Vec2.new(_spread[i].x * 1, _spread[i].y * 1), up)                

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
                    engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eShotgunShoot(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), newDirection * Vec3.new(2, 2, 2))
                    j = j + 1.0
                }

                i = i + 1
            }
            // Play shooting animation
            var gunAnimations = gun.GetAnimationControlComponent()
            gunAnimations.Play(_attackAnim, 0.0, 2.0, false)
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
        _range = 2
        _rangeVector = Vec3.new(_range, _range, _range)
        _attackSpeed = 0.2 * 1000
        _cooldown = 0
        _maxAmmo = 0
        _ammo = _maxAmmo
        _reloadTimer = 0
        _reloadSpeed = 0
        _cameraShakeIntensity = 0.2

        _attackSFX = "event:/Weapons/Machine Gun"
        _reloadSFX = ""
        _equipSFX = ""
        
        _attackAnim = "Shoot"
        _reloadAnim = "Reload"
        _equipAnim = "" 

        _mesh = ""
    }

    reload (engine) {
        // Use some weapon inspect animation maybe?
        System.print("Knife reload")
    }

    attack(engine, deltaTime, cameraVariables) {
        if (_cooldown <= 0) {
            System.print("Knife Stab")

            cameraVariables.shakeIntensity = _cameraShakeIntensity

            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName("AnimatedRifle")

            // Play shooting audio
            var eventInstance = engine.GetAudio().PlayEventOnce(_attackSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)

            // Spawn particles
            var playerTransform = player.GetTransformComponent()
            var translation = playerTransform.GetWorldTranslation()
            var rotation = playerTransform.GetWorldRotation()
            var direction = Math.ToVector(rotation)
            var up = rotation.mulVec3(Vec3.new(0, 1, 0))
            var right = Math.Cross(direction, up)
            var start = translation + direction * Vec3.new(0.01, 0.01, 0.01) - right * Vec3.new(0.1, 0.1, 0.1) - up * Vec3.new(0.1, 0.1, 0.1)
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, _range)

            var entity = engine.GetECS().NewEntity()
            var transform = entity.AddTransformComponent()
            transform.translation = start
            var lifetime = entity.AddLifetimeComponent()
            lifetime.lifetime = 100.0
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() | SpawnEmitterFlagBits.eEmitOnce() // |
            engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eStab(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), direction * Vec3.new(10, 10, 10))

            // Play shooting animation
            var gunAnimations = gun.GetAnimationControlComponent()
            gunAnimations.Play(_attackAnim, 0.0, 2.0, false)
            
            _cooldown = _attackSpeed
        } 
    }

    equip (engine) {
        // Knife should not be equipped?
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