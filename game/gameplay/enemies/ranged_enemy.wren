import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, PhysicsObjectLayer, RigidbodyComponent, CollisionShape, Math, Audio, SpawnEmitterFlagBits, EmitterPresetID, Random
import "../player.wren" for PlayerVariables

class RangedEnemy {

    construct new(engine, spawnPosition, size, maxSpeed, enemyModel, colliderShape) {
        
        _maxVelocity = maxSpeed

        _velocityDirection = Vec3.new(_maxVelocity, 0, 0)
        
        _meshEntity = engine.LoadModel(enemyModel)

        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "Enemy"
        _rootEntity.AddEnemyTag()
        _rootEntity.AddAudioEmitterComponent()
        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition
        transform.scale = size

        _rootEntity.AttachChild(_meshEntity)
        _meshEntity.GetTransformComponent().translation = Vec3.new(0,0,0)

        var rb = Rigidbody.new(engine.GetPhysics(), colliderShape, PhysicsObjectLayer.eENEMY(), true)
        var body = _rootEntity.AddRigidbodyComponent(rb)
        //body.SetStatic()
        body.SetGravityFactor(0.0)

        _isAlive = true

        _reasonTimer = 2001

        _attackRange = 80
        _attackDamage = 10
        _shakeIntensity = 1.6
        
        _movingState = false
        _attackingState = false
        _recoveryState = false
        _hitState = false

        _attackMaxCooldown = 10000
        _attackCooldown = _attackMaxCooldown

        _attackMaxTime = 4500
        _attackTime = 0
        _attackPauseTime = 50

        _recoveryMaxTime = 2000
        _recoveryTime = 0

        _evaluateState = true

        _health = 100

        _hitTimer = 0

        _chargeTimer = 0 // Interval between spawning white particle ray
        _attackPos = null

        _deathTimerMax = 1500
        _deathTimer = _deathTimerMax

        _shootSFX = "event:/SFX/EyeLaserBlast" 
        _chargeSFX = "event:/SFX/EyeLaserCharge"
        _hitSFX = "event:/SFX/EyeHit"

        _changeDirectionTimerMax = 2000
        _changeDirectionTimer = 0
        _dashTimer = 0
        _hasDashed = false
        _dashWishPosition = Vec3.new(0,0,0)

        _maxHeight = 32.0

        _hasTakenDamage = false
        _hasDashedFromDamage = false

        _chargeSoundEventInstance = null

    }

    IsHeadshot(y) { // Will probably need to be changed when we have a different model
        return false
    }

    DecreaseHealth(amount, engine) {
        var body = _rootEntity.GetRigidbodyComponent()
        _health = Math.Max(_health - amount, 0)

        var eventInstance = engine.GetAudio().PlayEventOnce(_hitSFX)
        engine.GetAudio().SetEventVolume(eventInstance, 20.0)
        _rootEntity.GetAudioEmitterComponent().AddEvent(eventInstance)
        // Fly some worms out of him
        var entity = engine.GetECS().NewEntity()
        var transform = entity.AddTransformComponent()
        transform.translation = body.GetPosition()
        var lifetime = entity.AddLifetimeComponent()
        lifetime.lifetime = 170.0
        var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
        engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eWorms(),emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(1,3.5, 1))

        if (_health <= 0 && _isAlive) {
            _isAlive = false
            _rootEntity.RemoveEnemyTag()
            
            body.SetVelocity(Vec3.new(0,0,0))
            
            //stop charging sound if it dies
            if(_chargeSoundEventInstance) {
                engine.GetAudio().StopEvent(_chargeSoundEventInstance)
                _chargeSoundEventInstance = null
            }
            body.SetDynamic()
            body.SetGravityFactor(2.0)

        } else {
            _rootEntity.GetRigidbodyComponent().SetVelocity(Vec3.new(0.0, 0.0, 0.0))
            //_hitState = true
            //_movingState = false
            //_evaluateState = false
            //_attackingState = false
            //_recoveryState = false
            _hasTakenDamage = true
            _hasDashedFromDamage = false
        }
    }

    health {_health}

    entity {
        return _rootEntity
    }

    position {
        return _rootEntity.GetTransformComponent().translation
    }

    position=(newPos) {
        _rootEntity.GetTransformComponent().translation = newPos
    }

    Update(playerPos, playerVariables, engine, dt, soulManager) {
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()
        _rootEntity.GetTransformComponent().translation = pos

        if (_isAlive) {
            if (_attackingState) {
                var forwardVector = Math.ToVector(_rootEntity.GetTransformComponent().rotation)
                if (_attackTime > 0 ) {    
                    _attackPos = playerPos
                    
                    // Rotate towards the player
                    var endRotation = Math.LookAt((pos - _attackPos), Vec3.new(0, 1, 0))
                    var startRotation = _rootEntity.GetTransformComponent().rotation
                    _rootEntity.GetTransformComponent().rotation = Math.Slerp(startRotation, endRotation, 0.03 *dt)

                    // Spawning white charging particles
                    _chargeTimer = _chargeTimer - dt
                    if (_chargeTimer <= 0) {
                        
                        _chargeTimer = 100
                        var start = pos
                        var direction = forwardVector
                        start = start + direction.mulScalar(2.0)
                        var end = pos + direction.mulScalar(_attackRange)
                        var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, _attackRange)
                        if (!rayHitInfo.isEmpty) {
                            end = rayHitInfo[0].position
                        }

                        var length = (end - start).length()
                        var j = 1.0
                        while (j < length) {
                            var entity = engine.GetECS().NewEntity()
                            var transform = entity.AddTransformComponent()
                            transform.translation = Math.MixVec3(start, end, j / length)
                            var lifetime = entity.AddLifetimeComponent()
                            lifetime.lifetime = 10.0
                            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                            engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eRayEyeStart(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), direction * Vec3.new(2, 2, 2))
                            j = j + 1.0
                        }
                    }
                    _attackTime = _attackTime - dt
                
                } else {

                    //stop charge sound if it's the case
                    if(_chargeSoundEventInstance) {
                        engine.GetAudio().StopEvent(_chargeSoundEventInstance)
                        _chargeSoundEventInstance = null
                    }
                    
                    // Raycast to try hit the player
                    var start = pos
                    var direction = forwardVector
                    start = start + direction.mulScalar(4.0)
                    var end = pos + direction.mulScalar(_attackRange)
                    var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, _attackRange)
                    if (!rayHitInfo.isEmpty) {
                        end = rayHitInfo[0].position
                        var player = engine.GetECS().GetEntityByName("PlayerController")
                        if (player && rayHitInfo[0].GetEntity(engine.GetECS()) == player) {
                            playerVariables.DecreaseHealth(_attackDamage)
                            playerVariables.cameraVariables.shakeIntensity = _shakeIntensity
                            playerVariables.invincibilityTime = playerVariables.invincibilityMaxTime
                        }
                    }

                    // Spawning particles along ray
                    var length = (end - start).length()
                    var j = 1.0
                    while (j < length) {
                        var entity = engine.GetECS().NewEntity()
                        var transform = entity.AddTransformComponent()
                        transform.translation = Math.MixVec3(start, end, j / length)
                        var lifetime = entity.AddLifetimeComponent()
                        lifetime.lifetime = 10.0
                        var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
                        engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eRayEyeEnd(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), direction * Vec3.new(2, 2, 2))
                        j = j + 1.0
                    }

                    var eventInstance = engine.GetAudio().PlayEventOnce(_shootSFX)
                    engine.GetAudio().SetEventVolume(eventInstance, 0.8)
                    _rootEntity.GetAudioEmitterComponent().AddEvent(eventInstance)


                    _attackingState = false
                    _recoveryState = true
                    _recoveryTime = _recoveryMaxTime
                    _attackCooldown = 0
                }
            }
            if (_recoveryState) {
                
                var forwardVector = (pos - playerPos).normalize()

                var endRotation = Math.LookAt(forwardVector, Vec3.new(0, 1, 0))
                var startRotation = _rootEntity.GetTransformComponent().rotation
                _rootEntity.GetTransformComponent().rotation = Math.Slerp(startRotation, endRotation, 0.01 *dt)

                _recoveryTime = _recoveryTime - dt
                if (_recoveryTime <= 0) {
                    _recoveryState = false
                    _evaluateState = true
                }
            }

            if (_movingState) {
                this.Move(playerPos, engine, dt)
                _evaluateState = true
            }

            if(_hasTakenDamage){
                _changeDirectionTimer = _changeDirectionTimerMax
                this.Move(playerPos, engine, dt)
            }

            if (_evaluateState) {
                _attackCooldown = _attackCooldown + dt
                if (Math.Distance(playerPos, pos) < _attackRange && _attackCooldown > _attackMaxCooldown) {
                    // Enter attack state
                    _attackingState = true
                    _movingState = false
                    _attackTime = _attackMaxTime
                    _evaluateState = false
                    
                    //play charge sound
                    _chargeSoundEventInstance = engine.GetAudio().PlayEventOnce(_chargeSFX)
                    engine.GetAudio().SetEventVolume(_chargeSoundEventInstance, 0.8)
                    _rootEntity.GetAudioEmitterComponent().AddEvent(_chargeSoundEventInstance)

                } else if (_movingState == false) { // Enter attack state
                    body.SetFriction(0.0)
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
                }
            }
        } else {
            _deathTimer = _deathTimer - dt

            var transparencyComponent = _meshEntity.GetTransparencyComponent()
            if(transparencyComponent==null) {
                transparencyComponent = _meshEntity.AddTransparencyComponent()
            }

            if (_deathTimer <= 0) {
                //spawn a soul
                soulManager.SpawnSoul(engine, body.GetPosition())
                engine.GetECS().DestroyEntity(_rootEntity) // Destroys the entity, and in turn this object
            } else {
                // Wait for death animation before starting descent
                transparencyComponent.transparency =  _deathTimer / (_deathTimerMax-100)
            }
        }
    }

    Move(playerPos, engine, dt) {
        _changeDirectionTimer = _changeDirectionTimer + dt
        
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()

        var forwardVector = (pos - playerPos).normalize()
        var endRotation = Math.LookAt(forwardVector, Vec3.new(0, 1, 0))
        var startRotation = _rootEntity.GetTransformComponent().rotation
        _rootEntity.GetTransformComponent().rotation = Math.Slerp(startRotation, endRotation, 0.01 *dt)

        if(_changeDirectionTimer > _changeDirectionTimerMax && !_hasDashedFromDamage){
            _hasDashed = true
            _changeDirectionTimer = 0
            var start = body.GetPosition()
            var direction = Random.RandomPointOnUnitSphere()
           
            if(pos.y > _maxHeight) {
                direction.y = -1.0
            } 

            var end = direction.mulScalar(20.0) + start
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, 20.0)
            _dashWishPosition = end
            if (!rayHitInfo.isEmpty) {
                for(hitInfo in rayHitInfo) {
                    if(hitInfo.GetEntity(engine.GetECS()) != _rootEntity) {
                        end = hitInfo.position
                        //add some offset to the end position based on the normal
                        end = end + hitInfo.normal.mulScalar(3.5)
                        _dashWishPosition = end
                        break
                    }
                }
            }
            _hasDashedFromDamage = true

        }

        if(_hasDashed){
            _dashTimer = _dashTimer + dt
            if(_dashTimer < 10000){
                var enemyBody = _rootEntity.GetRigidbodyComponent()
                enemyBody.SetTranslation(Math.MixVec3(enemyBody.GetPosition(), _dashWishPosition, 0.005))


                if(Math.Distance(enemyBody.GetPosition(), _dashWishPosition) < 2.0){
                    _dashTimer = 0
                    _hasTakenDamage = false
                    _hasDashedFromDamage = false
                    _changeDirectionTimer = 0
                    _hasDashed = false
                }
            }else{
                _hasDashed = false
                _hasDashedFromDamage = false
                _hasTakenDamage = false
                _dashTimer = 0
                _changeDirectionTimer = 0
            }
        }
        
    }

    FindNewPath(engine) {
       
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