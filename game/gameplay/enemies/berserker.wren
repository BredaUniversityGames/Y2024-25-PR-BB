import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, PhysicsObjectLayer, RigidbodyComponent, CollisionShape, Math, Audio, SpawnEmitterFlagBits, EmitterPresetID, Perlin, Random
import "../player.wren" for PlayerVariables

import "../soul.wren" for Soul, SoulManager, SoulType
import "../coin.wren" for Coin, CoinManager
import "gameplay/flash_system.wren" for FlashSystem

class BerserkerEnemy {

    construct new(engine, spawnPosition) {
        
        // ENEMY CONSTANTS
        _maxVelocity = 5
        var enemySize = 0.03
        var modelPath = "assets/models/Berserker.glb"
        var colliderShape = ShapeFactory.MakeCapsuleShape(145.0, 40.0) // TODO: Make this engine units
        
        _attackRange = 6.5
        _attackDamage = 35
        _shakeIntensity = 2.2
        _attackMaxTime = 2300
        _health = 26
        _attackTiming = 1200 // TODO: Should be added to small melee enemies
        _recoveryMaxTime = 500
        _deathTimerMax = 3000

        _growlSFX = "event:/SFX/DemonGrowl"
        _hurtSFX = "event:/SFX/DemonHurt"
        _stepSFX = "event:/SFX/DemonStep"
        _attackSFX = "event:/SFX/DemonAttack"
        _attackHitSFX = "event:/SFX/DemonAttackHit"
        _hitSFX = "event:/SFX/Hit"
        _spawnSFX = "event:/SFX/EnemySpawn"

        // ENTITY SETUP

        _meshEntity = engine.LoadModel(modelPath, false)
        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "BerserkerEnemy"
        _rootEntity.AddEnemyTag()
        _rootEntity.AddAudioEmitterComponent()
        
        _delaySpawnSFX = Random.RandomFloatRange(0, 1500)
        _playedSpawnSFX = false

        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition
        transform.scale = Vec3.new(enemySize, enemySize, enemySize)

        _rootEntity.AttachChild(_meshEntity)
        _meshEntity.GetTransformComponent().translation = Vec3.new(0,-121,0)
        _meshEntity.GetTransformComponent().scale = Vec3.new(1.4, 1.4, 1.4)

        _lightEntity = engine.GetECS().NewEntity()
        _lightEntity.AddNameComponent().name = "EnemyLight"
        var lightTransform = _lightEntity.AddTransformComponent()
        lightTransform.translation = Vec3.new(0, 12, 23)
        _pointLight = _lightEntity.AddPointLightComponent()
        _rootEntity.AttachChild(_lightEntity)

        _pointLight.intensity = 10
        _pointLight.range = 2
        _pointLight.color = Vec3.new(0.0, 1.0, 0.0)

        var rb = Rigidbody.new(engine.GetPhysics(), colliderShape, PhysicsObjectLayer.eENEMY(), false)
        var body = _rootEntity.AddRigidbodyComponent(rb)
        body.SetGravityFactor(2.2)

        var animations = _meshEntity.GetAnimationControlComponent()
        animations.Play("Walk", 0.4, true, 1.0, true)

        // STATE

        _currentPath = null
        _currentPathNodeIdx = null
        _honeInRadius = 30.0

        _isAlive = true

        _reasonTimer = 2000
        
        _movingState = false
        _attackingState = false
        _recoveryState = false
        _hitState = false
        _attackTime = _attackMaxTime
        _attackTimer = _attackTiming
        _recoveryTime = 0

        _evaluateState = true
        _hitTimer = 0
        _deathTimer = _deathTimerMax

        _walkEventInstance = null

        if(__perlin == null) {
            __baseIntensity = 10.0
            __flickerRange = 25.0
            __flickerSpeed = 1.0
            __perlin = Perlin.new(0)
        }
        _noiseOffset = 0.0
    }

    IsHeadshot(y) { // Will probably need to be changed when we have a different model
        if (y >= _rootEntity.GetRigidbodyComponent().GetPosition().y + 1.5) {
            return true
        }
        return false
    }

    DecreaseHealth(amount, engine, coinManager) {
        var animations = _meshEntity.GetAnimationControlComponent()
        var body = _rootEntity.GetRigidbodyComponent()

        _health = Math.Max(_health - amount, 0)

        if (_health <= 0 && _isAlive) {
            _isAlive = false
            _rootEntity.RemoveEnemyTag()
            animations.Play("Death", 1.0, false, 0.3, false)
            body.SetVelocity(Vec3.new(0,0,0))
            body.SetStatic()

            // Spawn between 1 and 5 coins
            var coinCount = Random.RandomIndex(7, 12)
            for(i in 0...coinCount) {
                coinManager.SpawnCoin(engine, body.GetPosition() + Vec3.new(0, 1.0, 0))
            }

            var eventInstance = engine.GetAudio().PlayEventOnce(_hurtSFX)
            var growlInstance = engine.GetAudio().PlayEventOnce(_growlSFX)
            var audioEmitter = _rootEntity.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)
            audioEmitter.AddEvent(growlInstance)    
        } else {
            //animations.Play("Hit", 1.0, false, 0.1, false)
            //_rootEntity.GetRigidbodyComponent().SetVelocity(Vec3.new(0.0, 0.0, 0.0))
            var hitmarkerSFX = engine.GetAudio().PlayEventOnce(_hitSFX)
            var eventInstance = engine.GetAudio().PlayEventOnce(_hurtSFX)
            var audioEmitter = _rootEntity.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)
            audioEmitter.AddEvent(hitmarkerSFX)
        }
    }

    health {_health}

    entity {
        return _rootEntity
    }

    position {
        return _rootEntity.GetTransformComponent().translation
    }

    Update(playerPos, playerVariables, engine, dt, soulManager, coinManager, flashSystem) {
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()
        var animations = _meshEntity.GetAnimationControlComponent()

    	_delaySpawnSFX = Math.Max(_delaySpawnSFX - dt, 0)
        if (!_playedSpawnSFX && _delaySpawnSFX <= 0) {
            var audioEmitter = _rootEntity.GetAudioEmitterComponent()
            audioEmitter.AddEvent(engine.GetAudio().PlayEventOnce(_spawnSFX))
            _playedSpawnSFX = true
        }

        if (_isAlive) {
            if (_attackingState) {
                _attackTime = _attackTime - dt
                _attackTimer = _attackTimer - dt

                if (_attackTimer <= 0 ) {
                    _rootEntity.GetAudioEmitterComponent().AddEvent(engine.GetAudio().PlayEventOnce(_attackHitSFX))
                    if (!playerVariables.IsInvincible()) {

                        var forward = Math.ToVector(_rootEntity.GetTransformComponent().rotation).mulScalar(-1)
                        var toPlayer = (playerPos - pos).normalize()

                        if (Math.Dot(forward, toPlayer) >= 0.8 && Math.Distance(playerPos, pos) < _attackRange) {
                            var rayHitInfo = engine.GetPhysics().ShootMultipleRays(pos, toPlayer, _attackRange, 3, 20)
                            var isOccluded = false
                            if (!rayHitInfo.isEmpty) {
                                for (rayHit in rayHitInfo) {
                                    var hitEntity = rayHit.GetEntity(engine.GetECS())
                                    if (hitEntity == _rootEntity || hitEntity.HasEnemyTag()) {
                                        continue
                                    }
                                    if (!hitEntity.HasPlayerTag()) {
                                        isOccluded = true
                                    }
                                    break
                                }
                            }

                            if (!isOccluded) {
                                playerVariables.DecreaseHealth(_attackDamage)
                                playerVariables.cameraVariables.shakeIntensity = _shakeIntensity
                                playerVariables.invincibilityTime = playerVariables.invincibilityMaxTime

                                playerVariables.hud.IndicateDamage(pos)
                                flashSystem.Flash(Vec3.new(1.0, 0.0, 0.0),0.85)
                                engine.GetAudio().PlayEventOnce(_hitSFX)
                            }
                        }
                    }

                    animations.Play("Idle", 1.0, true, 1.0, false)
                    animations.SetTime(0.0)
                    _attackTimer = 999999
                }

                if (_attackTime <= 0 ) {
                    _attackingState = false
                    _recoveryState = true
                    _recoveryTime = _recoveryMaxTime
                    _attackTime = _attackMaxTime
                    _attackTimer = _attackTiming
                }
            }
            if (_recoveryState) {
                if (animations.AnimationFinished()) {
                    animations.Play("Idle", 1.0, true, 1.0, false)
                    animations.SetTime(0.0)
                }
                var forwardVector = (playerPos - pos).normalize()

                var endRotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))
                var startRotation = body.GetRotation()
                body.SetRotation(Math.Slerp(startRotation, endRotation, 0.01 *dt))

                _recoveryTime = _recoveryTime - dt
                if (_recoveryTime <= 0) {
                    _recoveryState = false
                    _evaluateState = true
                }
            }

            if (_movingState) {
                this.DoPathfinding(playerPos, engine, dt)
                _evaluateState = true

                if(_walkEventInstance == null || engine.GetAudio().IsEventPlaying(_walkEventInstance) == false) {
                    _walkEventInstance = engine.GetAudio().PlayEventLoop(_stepSFX)
                    engine.GetAudio().SetEventVolume(_walkEventInstance, 1.0)
                    var audioEmitter = _rootEntity.GetAudioEmitterComponent()
                    audioEmitter.AddEvent(_walkEventInstance)
                }
            }else{
                if(_walkEventInstance && engine.GetAudio().IsEventPlaying(_walkEventInstance) == true) {
                    engine.GetAudio().StopEvent(_walkEventInstance)
                }
            }

            if (_evaluateState) {
                if (Math.Distance(playerPos, pos) < _attackRange) {
                    // Enter attack state
                    _attackingState = true
                    _movingState = false
                    body.SetFriction(12.0)
                    _attackTimer = _attackTiming 
                    if (animations.CurrentAnimationName() == "Walk") {
                        _attackTimer = _attackTimer - 100
                    }
                    animations.Play("Swipe", 1.0, false, 0.3, false)
                    animations.SetTime(0.0)
                    _attackTime = _attackMaxTime
                    _evaluateState = false

                    _rootEntity.GetAudioEmitterComponent().AddEvent(engine.GetAudio().PlayEventOnce(_attackSFX))

                } else if (_movingState == false) { // Enter attack state
                    body.SetFriction(0.0)
                    animations.Play("Walk", 0.4, true, 0.5, true)
                    _movingState = true
                }
            }

            if(_hitState) {
                _hitTimer = _hitTimer + dt

                if(_hitTimer > 1000) {
                    _hitTimer = 0
                    _movingState = true
                    _evaluateState = true
                    _hitState = false
                    body.SetDynamic()
                    animations.Play("Walk", 0.4, true, 0.5, true)
                }
            }
        } else {
            _deathTimer = _deathTimer - dt
            
            var transparencyComponent = _meshEntity.GetTransparencyComponent()
            if(transparencyComponent==null) {
                transparencyComponent = _meshEntity.AddTransparencyComponent()
            }

            if (_deathTimer <= 0) {
                soulManager.SpawnSoul(engine, body.GetPosition(),SoulType.BIG)
                engine.GetECS().DestroyEntity(_rootEntity) // Destroys the entity, and in turn this object
            } else {
                // Wait for death animation before starting descent
                if(_deathTimerMax - _deathTimer > 1800) {
                    transparencyComponent.transparency =  _deathTimer / (_deathTimerMax-1000)

                    var newPos = pos - Vec3.new(0, 1, 0).mulScalar(1.0 * 0.00075 * dt)
                    body.SetTranslation(newPos)
                }
            }
        }

        _noiseOffset = _noiseOffset + dt * 0.001 * __flickerSpeed
        var noise = __perlin.Noise1D(_noiseOffset)
        var flickerIntensity = __baseIntensity + ((noise - 0.5) * __flickerRange)
        _pointLight.intensity = flickerIntensity
    }

    DoPathfinding(playerPos, engine, dt) {
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()

        if(Math.Distance(position, engine.GetECS().GetEntityByName("Player").GetTransformComponent().GetWorldTranslation()) > _honeInRadius) {
            _reasonTimer = _reasonTimer + dt
            if(_reasonTimer > 2000) {
                this.FindNewPath(engine)
                _reasonTimer = 0
            }
        } else {
            _currentPath = null
            _reasonTimer = 2001
        }

        // Pathfinding logic
        if(_currentPath != null && _currentPath.GetWaypoints().count > 0) {
            var bias = 0.01
            var waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]

            if(Math.Distance(waypoint.center, pos) < 3.0 + bias) {
                _currentPathNodeIdx = _currentPathNodeIdx + 1
                if(_currentPathNodeIdx == _currentPath.GetWaypoints().count) {
                    body.SetVelocity(Vec3.new(0.0, 0.0, 0.0))
                    _currentPath = null
                    return
                }
                waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]
            }

            var p1 = _currentPath.GetWaypoints()[_currentPathNodeIdx]
            var p2 = p1
            if (_currentPathNodeIdx + 1 < _currentPath.GetWaypoints().count) {
                p2 = _currentPath.GetWaypoints()[_currentPathNodeIdx + 1]
            }

            var dst = Math.Distance(pos, p1.center)
            var target = Math.MixVec3(p1.center, p2.center, dst * bias)
            var forwardVector = (target - pos).normalize()

            _rootEntity.GetRigidbodyComponent().SetVelocity(forwardVector.mulScalar(_maxVelocity))
            
            // Set forward rotation
            var endRotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))
            var startRotation = body.GetRotation()
            body.SetRotation(Math.Slerp(startRotation, endRotation, 0.01 *dt))
        }else{
            var forwardVector = playerPos - position
            forwardVector.y = 0
            forwardVector = (forwardVector.normalize() + _rootEntity.GetRigidbodyComponent().GetVelocity())
            var maxVelocityScalar = 1.0
            if(forwardVector.length() >= _maxVelocity) {
                maxVelocityScalar = _maxVelocity / forwardVector.length()
            }

            var factor = Vec3.new(maxVelocityScalar, 1.0, maxVelocityScalar)
            _rootEntity.GetRigidbodyComponent().SetVelocity(forwardVector * factor)

            var endRotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))
            var startRotation = body.GetRotation()
            body.SetRotation(Math.Slerp(startRotation, endRotation, 0.01 *dt))
        }
    }

    FindNewPath(engine) {
        var startPos = position
        _currentPath = engine.GetPathfinding().FindPath(startPos, engine.GetECS().GetEntityByName("Player").GetTransformComponent().GetWorldTranslation())
        
        _currentPathNodeIdx = 1
    }

    Destroy(engine) {

        if(_rootEntity != null) {
            return
        }

        var ecs = engine.GetECS()
        ecs.DestroyEntity(_rootEntity)
        _rootEntity = null
    }

}