import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input, SpawnEmitterFlagBits, EmitterPresetID, PhysicsObjectLayer
import "camera.wren" for CameraVariables
import "player.wren" for PlayerVariables, HitmarkerState
import "station.wren" for PowerUpType, Station, StationManager


class Weapons {
    static pistol {0}
    static shotgun {1}
    static pistol2 {2}
}


class Pistol {
    construct new(engine, name, barrelEndPosition) {
        _damage = 1
        _headShotMultiplier = 2.0
        _range = 64
        _rangeVector = Vec3.new(_range, _range, _range)
        _attackSpeed = 0.4 * 1000    
        _manualTimer = 0
        _maxAmmo = 6
        _ammo = _maxAmmo
        _cooldown = 0
        _reloadTimer = 0
        _reloadSpeed = 0.8 * 1000

        _cameraShakeIntensity = 0.3

        _attackSFX = "event:/SFX/Revolver"
        _reloadSFX = "event:/SFX/ReloadPistol"
        _shotSFX = "event:/SFX/Shoot"
        _quadHit = "event:/SFX/QuadDamageHit"
        _dualGunHit = "event:/SFX/DualGunHit"
        _equipSFX = ""
        __hitmarkTimer = 0
        _walkAnim = "walk"
        _idleAnim = "idle"
        _attackAnim = "shoot"
        _reloadAnim = "reload"
        _equipAnim = "equip"
        _unequipAnim = "unequip" 
        _entityName = name 

        _barrelEndPosition = barrelEndPosition

        var gun = engine.GetECS().GetEntityByName(_entityName)
        _barrelEndEntity = engine.GetECS().NewEntity()
        _barrelEndEntity.AddNameComponent().name = "BarrelEnd %(_entityName)"
        var transform = _barrelEndEntity.AddTransformComponent()
        gun.AttachChild(_barrelEndEntity)
        transform.translation = _barrelEndPosition

        var finalName = _barrelEndEntity.GetNameComponent().name

        _mesh = ""
    }

    reload (engine) {
        var gun = engine.GetECS().GetEntityByName(_entityName)

        var gunAnimations = gun.GetAnimationControlComponent()
        
        var shootBool = false

        if (_entityName == "Gun" && engine.GetInput().GetDigitalAction("Shoot").IsHeld()) {
            shootBool = true
        }

        if (_entityName == "Gun2" && engine.GetInput().GetDigitalAction("ShootSecondary").IsHeld()) {
            shootBool = true
        }

        if(engine.GetInput().GetDigitalAction("Reload").IsPressed() || shootBool  && _reloadTimer == 0) {
            gunAnimations.Play(_reloadAnim, 1.0, false, 0.2, false)

            // Play reload audio
            var player = engine.GetECS().GetEntityByName("Camera")
            var playerController = engine.GetECS().GetEntityByName("PlayerController")
            var rb =  playerController.GetRigidbodyComponent()
            var velocity = rb.GetVelocity()

            var eventInstance = engine.GetAudio().PlayEventOnce(_reloadSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)

            var gunTransform = gun.GetTransformComponent()
            var gunTranslation = gunTransform.GetWorldTranslation()
            var gunRotation = gunTransform.GetWorldRotation()
            var gunForward = Math.ToVector(gunRotation)
            var gunUp = gunRotation.mulVec3(Vec3.new(0, 1, 0))
            var gunRight = Math.Cross(gunForward, gunUp)
            //var gunStart = gunTranslation + gunForward * Vec3.new(1, 1, 1) - gunRight * Vec3.new(4.0,4.0,4.0) - gunUp * Vec3.new(0.0, 0.5, 0.0)
            var gunStart = _barrelEndEntity.GetTransformComponent().GetWorldTranslation()

            //play a particle effect
            var entity = engine.GetECS().NewEntity()
            var transform = entity.AddTransformComponent()
            transform.translation = Vec3.new(gunStart.x, gunStart.y-0.5, gunStart.z)
            var lifetime = entity.AddLifetimeComponent()
            lifetime.lifetime = 175.0
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eBullets(),emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.1, 7.5, 0.1) + velocity.mulScalar(1.01))


            _reloadTimer = _reloadSpeed
            _ammo = _maxAmmo
        }
    }

    playWalkAnim (engine){
        //will hold reference to entity when implementing weapon switching
        var gun = engine.GetECS().GetEntityByName(_entityName)
        var gunAnimations = gun.GetAnimationControlComponent()
        if(gunAnimations.AnimationFinished() || gunAnimations.CurrentAnimationName() == _idleAnim){
            gunAnimations.Play(_walkAnim, 1.0, false, 0.2, false)
        }
    }

    playIdleAnim(engine){
        var gun = engine.GetECS().GetEntityByName(_entityName)
        var gunAnimations = gun.GetAnimationControlComponent()
        if(gunAnimations.AnimationFinished() || gunAnimations.CurrentAnimationName() == _walkAnim){
            gunAnimations.Play(_idleAnim, 1.0, false, 0.2, false)
        } 
    }

    attack(engine, deltaTime, playerVariables, enemies, coinManager) {
        
        _manualTimer = Math.Max(_manualTimer-deltaTime,0)
        
        if (_entityName == "Gun") {
                if(engine.GetInput().GetDigitalAction("Shoot").IsPressed() && _manualTimer ==0){
                _manualTimer = 50 //ms
            }
        }
        if (_entityName == "Gun2") {
                if(engine.GetInput().GetDigitalAction("ShootSecondary").IsPressed() && _manualTimer ==0){
                _manualTimer = 50 //ms
            }
        }


        if ((_cooldown <= 0 ||_manualTimer >=50) && _ammo > 0 && _reloadTimer <= 0) {
            _ammo = _ammo - 1

            // Shake the camera
            playerVariables.cameraVariables.shakeIntensity = _cameraShakeIntensity            
            playerVariables.cameraVariables.AddRecoil(3)
    
            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName(_entityName)
            
            
            // Play shooting audio
            var eventInstance = engine.GetAudio().PlayEventOnce(_shotSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            
            // Play quad damage audio if needed
            if(playerVariables.GetCurrentPowerUp() == PowerUpType.QUAD_DAMAGE){
                var quadEventInstance = engine.GetAudio().PlayEventOnce(_quadHit)
                engine.GetAudio().SetEventVolume(quadEventInstance, 3.0)

                audioEmitter.AddEvent(quadEventInstance)
            }

            if(playerVariables.GetCurrentPowerUp() == PowerUpType.DOUBLE_GUNS){
                var dualEventInstance = engine.GetAudio().PlayEventOnce(_dualGunHit)
                engine.GetAudio().SetEventVolume(dualEventInstance, 2.0)

                audioEmitter.AddEvent(dualEventInstance)
            }   

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
                var normal = Vec3.new(0, 1, 0)
                for (rayHit in rayHitInfo) {
                    var hitEntity = rayHit.GetEntity(engine.GetECS())
                    if (!hitEntity.HasPlayerTag()) {
                        end = rayHit.position
                        normal = rayHit.normal
                        if (hitEntity.HasEnemyTag()) {
                            for (enemy in enemies) {
                                if (enemy.entity == hitEntity) {
                                    
                                    var multiplier = 1.0

                                    if (enemy.IsHeadshot(rayHit.position.y)) {
                                        multiplier = _headShotMultiplier
                                        // Critical hitmarker
                                        playerVariables.hitmarkTimer = 200 //ms
                                        playerVariables.hitmarkerState = HitmarkerState.crit
                                    }else{
                                        // Normal hitmarker
                                        playerVariables.hitmarkTimer = 200 //ms
                                        playerVariables.hitmarkerState = HitmarkerState.normal
                                    }
                                    playerVariables.UpdateMultiplier()
                                    enemy.DecreaseHealth(_damage * multiplier * playerVariables.GetDamageMultiplier(),engine,coinManager)
                                    if (enemy.health <= 0) {
                                        playerVariables.IncreaseScore(5 * multiplier * playerVariables.multiplier)
                                        //playerVariables.UpdateUltCharge(1.0)
                                    }
                                }
                            }
                            break
                        }
                        if(hitEntity.GetRigidbodyComponent().GetLayer() == PhysicsObjectLayer.eSTATIC()) {
                            engine.SpawnDecal(normal, end, Vec2.new(0.001, 0.001), "bullet_hole.png")
                        }
                        break
                    }
                }

                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = end
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 400.0

                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eImpact(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), normal.mulScalar(0.005) + Vec3.new(0.0, 0.4, 0.0))
            }

            var gunStart = _barrelEndEntity.GetTransformComponent().GetWorldTranslation()

            var length = (end - gunStart).length()
            var i = 1.0
            while (i < length) {
                var entity = engine.GetECS().NewEntity()
                var transform = entity.AddTransformComponent()
                transform.translation = Math.MixVec3(gunStart, end, i / length)
                var lifetime = entity.AddLifetimeComponent()
                lifetime.lifetime = 200.0
                var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                engine.GetParticles().SpawnEmitter(entity, playerVariables.GetGunSmokeRay(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), direction * Vec3.new(10, 10, 10))
                i = i + 2.0
            }

            // Play shooting animation
            var gunAnimations = gun.GetAnimationControlComponent()
            gunAnimations.Play(_attackAnim, 1.0, false, 0.0, false)

            _cooldown = _attackSpeed
        }
    }

    equip (engine) {
        engine.GetECS().DestroyEntity(engine.GetECS().GetEntityByName(_entityName))
        if(_barrelEndEntity.IsValid()){
            engine.GetECS().DestroyEntity(_barrelEndEntity)
        }

        var camera = engine.GetECS().GetEntityByName("Camera")

        var newGun = engine.LoadModel("assets/models/Revolver.glb",false)
        newGun.GetNameComponent().name = _entityName
        var gunTransform = newGun.GetTransformComponent()
        gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI()/2, 0.0))

        camera.AttachChild(newGun)
        var gunAnimations =newGun .GetAnimationControlComponent()
        gunAnimations.Play(_equipAnim, 1.2, false, 0.2, false)

        newGun.RenderInForeground()

        // Create barrel end entity
        var gun = engine.GetECS().GetEntityByName(_entityName)
        _barrelEndEntity = engine.GetECS().NewEntity()
        _barrelEndEntity.AddNameComponent().name = "BarrelEnd %(_entityName)"
        var transform = _barrelEndEntity.AddTransformComponent()
        gun.AttachChild(_barrelEndEntity)
        transform.translation = _barrelEndPosition

    }

    unequip(engine){
        var gunAnimations = engine.GetECS().GetEntityByName(_entityName).GetAnimationControlComponent()
        gunAnimations.Play(_unequipAnim, 1.5, false, 0.2, false)
    }

    isUnequiping(engine){

        var gunAnimations =engine.GetECS().GetEntityByName(_entityName).GetAnimationControlComponent()
        return gunAnimations.CurrentAnimationName() == _unequipAnim || gunAnimations.CurrentAnimationName() == _equipAnim
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
        _damage = 15
        _damageDropoff = 0.5
        _raysPerShot = 9
        _range = 128
        _rangeVector = Vec3.new(_range, _range, _range)
        _attackSpeed = 0.3 * 1000
        _maxAmmo = 2
        _ammo = _maxAmmo
        _cooldown = 0
        _reloadTimer = 0
        _reloadSpeed = 600
        _spread = [Vec2.new(0, 0), Vec2.new(-1, 1), Vec2.new(0, 1), Vec2.new(1, 1), Vec2.new(0, 2), Vec2.new(-1, -1), Vec2.new(0, -1), Vec2.new(1, -1), Vec2.new(0, -2)]
        _cameraShakeIntensity = 0.5

        _attackSFX = "event:/SFX/Shotgun"
        _reloadSFX = "event:/SFX/ShotgunReload"
        _equipSFX = ""

        _walkAnim = "walk"
        _idleAnim = "idle"
        _attackAnim = "shoot"
        _reloadAnim = "reload"
        _equipAnim = "equip" 
        _unequipAnim = "unequip"
        _entityName = "Gun" 
        _mesh = ""
    }

    reload (engine) {
        if((engine.GetInput().GetDigitalAction("Reload").IsPressed() || engine.GetInput().GetDigitalAction("Shoot").IsHeld()) && _reloadTimer == 0) {
            var gun = engine.GetECS().GetEntityByName(_entityName)

            var gunAnimations = gun.GetAnimationControlComponent()
            gunAnimations.Play(_reloadAnim, 1.0, false, 0.2, false)
             _reloadTimer = _reloadSpeed
            _ammo = _maxAmmo

            var player = engine.GetECS().GetEntityByName("Camera")
            var eventInstance = engine.GetAudio().PlayEventOnce(_reloadSFX)
            var audioEmitter = player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)
        }
    }

    attack(engine, deltaTime, playerVariables, enemies, coinManager) {
        if (_cooldown <= 0 && _ammo > 0 && _reloadTimer <= 0) {
            _ammo = _ammo - 1
 
            playerVariables.cameraVariables.shakeIntensity = _cameraShakeIntensity            
            var player = engine.GetECS().GetEntityByName("Camera")
            var gun = engine.GetECS().GetEntityByName(_entityName)

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

            var hitAnEnemy = false
          
            var i = 0
            while (i < _raysPerShot) {
                var newDirection = Math.RotateForwardVector(direction, Vec2.new(_spread[i].x * 1, _spread[i].y * 1), up)
                var rayHitInfo = engine.GetPhysics().ShootRay(start, newDirection, _range)
                var end = start + newDirection * _rangeVector

                if (!rayHitInfo.isEmpty) {
                    for (rayHit in rayHitInfo) {
                        var hitEntity = rayHit.GetEntity(engine.GetECS())
                        if (!hitEntity.HasPlayerTag()) {
                            end = rayHit.position
                            if (hitEntity.HasEnemyTag()) {
                                for (enemy in enemies) {
                                    if (enemy.entity == hitEntity) {
                                        hitAnEnemy = true
                                        
                                        playerVariables.hitmarkTimer = 200 //ms      
                                        enemy.DecreaseHealth(_damage,engine,coinManager)
                                        
                                        playerVariables.multiplierTimer = playerVariables.multiplierMaxTime
                                        playerVariables.IncreaseHealth(0.1 * _damage)
                                        if (enemy.health <= 0) {
                                            playerVariables.IncreaseScore(15 * playerVariables.multiplier)
                                            //playerVariables.UpdateUltCharge(0.1) // Allow the player to try and keep the ult active a bit longer
                                        }
                                    }
                                }
                                break
                            }
                            break
                        }
                    }
                }

                var length = (end - start).length()
                var j = 1.0
                while (j < length) {
                    var entity = engine.GetECS().NewEntity()
                    var transform = entity.AddTransformComponent()
                    transform.translation = Math.MixVec3(start, end, j / length)
                    var lifetime = entity.AddLifetimeComponent()
                    lifetime.lifetime = 200.0
                    var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                    engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eShotgunShoot(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), newDirection * Vec3.new(2, 2, 2))
                    j = j + 1.0
                }

                i = i + 1
            }

            if (hitAnEnemy) {
                playerVariables.UpdateMultiplier()
            }

            // Play shooting animation
            var gunAnimations = gun.GetAnimationControlComponent()
            gunAnimations.Play(_attackAnim, 1.1, false, 0.0, false)
            _cooldown = _attackSpeed
        }
    }

    equip (engine) {
        engine.GetECS().DestroyEntity(engine.GetECS().GetEntityByName(_entityName))

        var camera = engine.GetECS().GetEntityByName("Camera")

        var newGun =  engine.LoadModel("assets/models/Shotgun.glb", false)
        newGun.GetNameComponent().name = _entityName
        var gunTransform = newGun.GetTransformComponent()
        gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI()/2, 0.0))

        camera.AttachChild(newGun)
        var gunAnimations =newGun .GetAnimationControlComponent()
        gunAnimations.Play(_equipAnim, 1.2, false, 0.0, false)
        newGun.RenderInForeground()
    }

    isUnequiping(engine){
        var gunAnimations = engine.GetECS().GetEntityByName(_entityName).GetAnimationControlComponent()
        return gunAnimations.CurrentAnimationName() == _unequipAnim || gunAnimations.CurrentAnimationName() == _equipAnim
    }

    unequip(engine){
        var gunAnimations = engine.GetECS().GetEntityByName(_entityName).GetAnimationControlComponent()
        gunAnimations.Play(_unequipAnim, 1.5, false, 0.0, false)
    }

    playWalkAnim (engine){
        //will hold reference to entity when implementing weapon switching
        var gun = engine.GetECS().GetEntityByName(_entityName)
        var gunAnimations = gun.GetAnimationControlComponent()
        if(gunAnimations.AnimationFinished() || (gunAnimations.CurrentAnimationName() == _idleAnim || gunAnimations.CurrentAnimationName() == "slide")){
            gunAnimations.Play(_walkAnim, 1.0, false, 0.2, false)
        }
    }

    playIdleAnim(engine){
        var gun = engine.GetECS().GetEntityByName(_entityName)
        var gunAnimations = gun.GetAnimationControlComponent()
        if(gunAnimations.AnimationFinished() || (gunAnimations.CurrentAnimationName() == _walkAnim || gunAnimations.CurrentAnimationName() == "slide") ){
            gunAnimations.Play(_idleAnim, 1.0, false, 0.2, false)
        }
    }

    cooldown {_cooldown}
    cooldown=(value) {_cooldown = value}

    reloadTimer {_reloadTimer}
    reloadTimer=(value) {_reloadTimer = value}

    ammo {_ammo}
    ammo=(value) {_ammo = value}

    maxAmmo {_maxAmmo}
}