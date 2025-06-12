import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, PhysicsObjectLayer, RigidbodyComponent, CollisionShape, Math, Audio, SpawnEmitterFlagBits, Perlin, Random, Stat, Stats
import "../player.wren" for PlayerVariables
import "../soul.wren" for Soul, SoulManager, SoulType
import "../coin.wren" for Coin, CoinManager
import "gameplay/flash_system.wren" for FlashSystem
import "../station.wren" for PowerUpType

class MeleeEnemy {

    construct new(engine, spawnPosition, waveNumber) {
        
        // ENEMY CONSTANTS
        _maxVelocity = 13
        _attackRange = 4.5
        _attackDamage = 20
        _shakeIntensity = 1.6
        _attackCooldown = 2000
        _recoveryMaxTime = 1000
        _health = 4
        _deathTimerMax = 3000
        _getUpTimer = 3500
        _getUpAppearMax = 1500
        _attackTiming = 1460
        _attackTimer = _attackTiming
        _reasonTimeout = 2000
        _honeInRadius = 30.0
        _honeInMaxAltitude = 4.0

        _bonesSFX = "event:/SFX/Bones"
        _hitMarkerSFX = "event:/SFX/Hitmarker"
        _bonesStepsSFX = "event:/SFX/BonesSteps"
        _roar = "event:/SFX/Roar"
        _hitSFX = "event:/SFX/Hit"
        _spawnSFX = "event:/SFX/EnemySpawn"

        var enemySize = 0.0165
        var modelPath = "assets/models/Skeleton.glb"
        var colliderShape = ShapeFactory.MakeCapsuleShape(90.0, 40.0) // TODO: Make this engine units

        // PATHFINDING
        _currentPath = null
        _currentPathNodeIdx = null


        // ENTITY SETUP

        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "Enemy"
        _rootEntity.AddEnemyTag()
        _rootEntity.AddAudioEmitterComponent()
        
        _delaySpawnSFX = Random.RandomFloatRange(0, 1500)
        _playedSpawnSFX = false

        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition
        transform.scale = Vec3.new(enemySize, enemySize, enemySize)

        var rotation = Vec3.new(0.0, Random.RandomFloatRange(0.0, 3.14 * 2.0), 0.0)
        transform.rotation = Math.ToQuat(rotation)

        _meshEntity = engine.LoadModel(modelPath, false)
        _meshEntity.GetTransformComponent().translation = Vec3.new(0,-60,0)
        _rootEntity.AttachChild(_meshEntity)

        _lightEntity = engine.GetECS().NewEntity()
        _lightEntity.AddNameComponent().name = "EnemyLight"
        var lightTransform = _lightEntity.AddTransformComponent()
        lightTransform.translation = Vec3.new(0.0, 26, 0.0)
        _pointLight = _lightEntity.AddPointLightComponent()
        _rootEntity.AttachChild(_lightEntity)

        var transparencyComponent = _meshEntity.AddTransparencyComponent()

        _pointLight.intensity = 10
        _pointLight.range = 2
        _pointLight.color = Vec3.new(Math.Min(waveNumber, 10) / 10, 1.0 - Math.Min(waveNumber, 10) / 10, 0.0)

        var rb = Rigidbody.new(engine.GetPhysics(), colliderShape, PhysicsObjectLayer.eENEMY(), false)
        var body = _rootEntity.AddRigidbodyComponent(rb)
        body.SetGravityFactor(2.2)

        var animations = _meshEntity.GetAnimationControlComponent()
        animations.Play("Stand-up", 1.0, false, 0.0, false)

        // STATE
        
        _isAlive = true
        _reasonTimer = _reasonTimeout
        _getUpState = true
        _movingState = false
        _attackingState = false
        _recoveryState = false
        _hitState = false
        _attackTime = _attackCooldown
        _recoveryTime = 0
        _evaluateState = true
        _hitTimer = 0
        _getUpAppearTimer = _getUpAppearMax
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
        if (y >= _rootEntity.GetRigidbodyComponent().GetPosition().y + 0.5 && !_getUpState) {
            return true
        }
        return false
    }

    DecreaseHealth(amount, engine, coinManager, playerVariables) {
        var animations = _meshEntity.GetAnimationControlComponent()
        var body = _rootEntity.GetRigidbodyComponent()

        _health = Math.Max(_health - amount, 0)

        // Fly some bones out of him
        var entity = engine.GetECS().NewEntity()
        var transform = entity.AddTransformComponent()
        transform.translation = body.GetPosition()
        var lifetime = entity.AddLifetimeComponent()
        lifetime.lifetime = 170.0
        var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
        engine.GetParticles().SpawnEmitter(entity, "Bones",emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.0, 15.0, 0.0))

        if (_health <= 0 && _isAlive) {
            _isAlive = false
            _rootEntity.RemoveEnemyTag()
            animations.Play("Death", 1.0, false, 0.3, false)
            body.SetLayer(PhysicsObjectLayer.eDEAD())
            body.SetVelocity(Vec3.new(0,0,0))
            body.SetStatic()
            // Spawn between 1 and 5 coins
            var coinCount = Random.RandomIndex(2, 5)
            for(i in 0...coinCount) {
                coinManager.SpawnCoin(engine, body.GetPosition() + Vec3.new(0, 1.0, 0))
            }

            var stat = engine.GetSteam().GetStat(Stats.SKELETONS_KILLED())
            stat.intValue = stat.intValue + 1

            var playerPowerUp = playerVariables.GetCurrentPowerUp()
            if(playerPowerUp != PowerUpType.NONE) {
                var powerUpStat = engine.GetSteam().GetStat(Stats.ENEMIES_KILLED_WITH_RELIC())
                powerUpStat.intValue = powerUpStat.intValue + 1
            }

            _pointLight.intensity = 0

            var eventInstance = engine.GetAudio().PlayEventOnce(_bonesSFX)
            
            var hitmarkerSFX = engine.GetAudio().PlayEventOnce(_hitMarkerSFX)
            var audioEmitter = _rootEntity.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)
            audioEmitter.AddEvent(hitmarkerSFX)
        } else {
            animations.Play("Hit", 1.0, false, 0.1, false)
            _rootEntity.GetRigidbodyComponent().SetVelocity(Vec3.new(0.0, 0.0, 0.0))
            _hitState = true
            _movingState = false
            _evaluateState = false
            _attackingState = false
            _recoveryState = false
            body.SetStatic()

            var hitmarkerSFX = engine.GetAudio().PlayEventOnce(_hitMarkerSFX)
            var eventInstance = engine.GetAudio().PlayEventOnce(_bonesSFX)
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
        var transparencyComponent = _meshEntity.GetTransparencyComponent()

        _delaySpawnSFX = Math.Max(_delaySpawnSFX - dt, 0)
        if (!_playedSpawnSFX && _delaySpawnSFX <= 0) {
            var audioEmitter = _rootEntity.GetAudioEmitterComponent()
            audioEmitter.AddEvent(engine.GetAudio().PlayEventOnce(_spawnSFX))
            _playedSpawnSFX = true
        }

        if (_isAlive) {
            if (_getUpState) {
                _getUpTimer = _getUpTimer - dt
                _getUpAppearTimer = _getUpAppearTimer - dt

                transparencyComponent.transparency =  1.0 - _getUpAppearTimer / _getUpAppearMax

                if(_getUpTimer < 0) {
                    _getUpState = false
                    transparencyComponent.transparency = 1.0
                }

                return
            }

            if (_attackingState) {
                _attackTime = _attackTime - dt
                _attackTimer = _attackTimer - dt

                if (_attackTimer <= 0 ) {
                    _attackTimer = 999999
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

                                //Flash the screen red
                                flashSystem.Flash(Vec3.new(105 / 255, 13 / 255, 1 / 255),0.75)

                                playerVariables.hud.IndicateDamage(pos)
                                engine.GetAudio().PlayEventOnce(_hitSFX)
                                //animations.Play("Attack", 1.0, false, 0.1, false)
                            }
                        }
                    }
                }

                if (_attackTime <= 0) {
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
                body.SetRotation( Math.Slerp(startRotation, endRotation, 0.01 *dt))

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
                    _walkEventInstance = engine.GetAudio().PlayEventLoop(_bonesStepsSFX)
                    engine.GetAudio().SetEventVolume(_walkEventInstance, 8.0)
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

                    if (animations.CurrentAnimationName() == "Run") {
                        _attackTimer = _attackTimer - 450
                    }

                    animations.Play("Attack", 1.0, false, 0.3, false)
                    animations.SetTime(0.0)
                    _attackTime = _attackCooldown
                    _evaluateState = false
                    _rootEntity.GetAudioEmitterComponent().AddEvent(engine.GetAudio().PlayEventOnce(_roar))

                } else if (_movingState == false) {
                    body.SetFriction(0.0)
                    animations.Play("Run", 1.25, true, 0.2, true)
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
                    animations.Play("Run", 1.25, true, 0.2, true)
                }
            }

            _noiseOffset = _noiseOffset + dt * 0.001 * __flickerSpeed
            var noise = __perlin.Noise1D(_noiseOffset)
            var flickerIntensity = __baseIntensity + ((noise - 0.5) * __flickerRange)
            _pointLight.intensity = flickerIntensity
        } else {
            _deathTimer = _deathTimer - dt
            
            if (_deathTimer <= 0) {
                //spawn a soul
                soulManager.SpawnSoul(engine, body.GetPosition(),SoulType.SMALL)

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
    }

    DoPathfinding(playerPos, engine, dt) {
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()

        var distToPlayer = Math.Distance(pos, playerPos)
        var altitudeToPlayer = Math.Distance(Vec3.new(0.0, pos.y, 0.0), Vec3.new(0.0, playerPos.y, 0.0))
        var forwardVector = Vec3.new(0.0, 0.0, 0.0)

        if(distToPlayer > _honeInRadius || altitudeToPlayer > _honeInMaxAltitude) {
            _reasonTimer = _reasonTimer + dt
            if(_reasonTimer > _reasonTimeout) {
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
            forwardVector = target - pos
        }else{
            forwardVector = (playerPos - position).normalize()
            var localSteerForward = this.LocalSteer(engine, pos, forwardVector)
            if(localSteerForward) {
                forwardVector = localSteerForward
            }
        }

        forwardVector = forwardVector.normalize()
        forwardVector = (_rootEntity.GetRigidbodyComponent().GetVelocity() + forwardVector).normalize()
        _rootEntity.GetRigidbodyComponent().SetVelocity(forwardVector.mulScalar(_maxVelocity))

        var endRotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))
        var startRotation = body.GetRotation()
        body.SetRotation(Math.Slerp(startRotation, endRotation, 0.01 *dt))
    }

    FindNewPath(engine) {
        var startPos = position
        _currentPath = engine.GetPathfinding().FindPath(startPos, engine.GetECS().GetEntityByName("Player").GetTransformComponent().GetWorldTranslation())
        
        _currentPathNodeIdx = 1
    }

    // Returns a corrected vector or null if no objects were found
    LocalSteer(engine, position, forwardVector) {
        var seekDepth = 3.0
        var rayCount = 6
        var rayAngle = 20 // degrees

        var offsetToKnees = Vec3.new(0.0, -0.25, 0.0)

        var angleStep = Math.Radians(rayAngle) / (rayCount / 2)

        var offsetDirection = Vec3.new(0, 0, 0)

        // Find closest ray hit and add this to the offsetDirection vector
        for(i in 0...rayCount) {
            var angleOffset = (i-(rayCount - 1) / 2.0) * angleStep
            var rotatedDirection = Math.RotateY(forwardVector, angleOffset)

            var hitInfos = engine.GetPhysics().ShootRay(position + offsetToKnees, rotatedDirection, seekDepth)
            var lowestHitFraction = 1.0
            var lowestHitFractionIndex = 0
            for(j in 0...hitInfos.count) {

                var ray = hitInfos[j]

                if(ray.GetEntity(engine.GetECS()).GetEnttEntity() == _rootEntity.GetEnttEntity()) {
                    continue
                }

                if(ray.hitFraction < lowestHitFraction) {
                    lowestHitFraction = ray.hitFraction
                    lowestHitFractionIndex = j
                }
            }

            offsetDirection = offsetDirection + (position - hitInfos[lowestHitFractionIndex].position)
        }

        // If we hit nothing
        if(offsetDirection.length() < 0.001) {
            return null
        }

        offsetDirection = offsetDirection.normalize()
        var absDot = Math.Abs(Math.Dot(forwardVector, offsetDirection))

        if(absDot > 0.9) {
            // if the raycast results is a vector that is nearly a negated forward vector
            // steer either hard left or right (random chance for either)

            var angle = 0.0

            var index = Random.RandomIndex(0, 1)
            if(index == 0) {
                angle = -90.0
            } else {
                angle = 90.0
            }

            forwardVector = Math.RotateY(forwardVector, angle)
        } else {
            if(absDot < 0.001) {
                // if no hits (or too subtle) keep same forward vector
            } else {
                // regular steering behavior
                forwardVector = forwardVector - offsetDirection
            }
        }

        return forwardVector
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