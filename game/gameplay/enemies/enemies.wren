import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Math, Audio
import "../player.wren" for PlayerVariables

class MeleeEnemy {

    construct new(engine, spawnPosition, size, maxSpeed, enemyModel, colliderShape) {
        
        _maxVelocity = maxSpeed
        _currentPath = null
        _currentPathNodeIdx = null

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
        _meshEntity.GetTransformComponent().translation = Vec3.new(0,-100,0)

        var rb = Rigidbody.new(engine.GetPhysics(), colliderShape, true, false)
        var body = _rootEntity.AddRigidbodyComponent(rb)
        // body.SetFriction(2.0)

        var animations = _meshEntity.GetAnimationControlComponent()
        animations.Play("Run", 1.0, true, 1.0, true)

        _isAlive = true

        _reasonTimer = 0.0

        _attackRange = 6
        _attackDamage = 30
        _shakeIntensity = 1.6
        
        _movingState = false
        _attackingState = false
        _recoveryState = false

        _attackMaxCooldown = 2000
        _attackCooldown = _attackMaxCooldown

        _attackMaxTime = 1300
        _attackTime = 0

        _recoveryMaxTime = 2000
        _recoveryTime = 0

        _evaluateState = true

        _health = 100

        _deathTimer = 1000
    }

    DecreaseHealth(amount) {
        _health = Math.Max(_health - amount, 0)
        if (_health <= 0 && _isAlive) {
            _isAlive = false
            _rootEntity.RemoveEnemyTag()
            var animations = _meshEntity.GetAnimationControlComponent()
            animations.Play("Death", 1.0, true, 1.0, false)
            var body = _rootEntity.GetRigidbodyComponent()
            body.SetVelocity(Vec3.new(0,0,0))
            body.SetStatic()
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

    Update(playerPos, playerVariables, engine, dt) {
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()
        _rootEntity.GetTransformComponent().translation = pos
        var animations = _meshEntity.GetAnimationControlComponent()

        if (_isAlive) {
            if (_attackingState) {
                _attackTime = _attackTime - dt
                if (_attackTime <= 0 ) {
                    if (Math.Distance(playerPos, pos) < _attackRange && !playerVariables.IsInvincible()) {
                        playerVariables.DecreaseHealth(_attackDamage)
                        playerVariables.cameraVariables.shakeIntensity = _shakeIntensity
                        playerVariables.invincibilityTime = playerVariables.invincibilityMaxTime

                        engine.GetAudio().PlaySFX("assets/sounds/hit1.wav", 1.0)
                    }

                    System.print("Enter Recovery State")
                    _attackingState = false
                    _recoveryState = true
                    _recoveryTime = _recoveryMaxTime 
                }
            }
            if (_recoveryState) {
                if (animations.AnimationFinished()) {
                    animations.Play("Idle", 1.0, true, 1.0, false)
                    animations.SetTime(0.0)
                }
                var forwardVector = (playerPos - pos).normalize()

                var endRotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))
                var startRotation = _rootEntity.GetTransformComponent().rotation
                _rootEntity.GetTransformComponent().rotation = Math.Slerp(startRotation, endRotation, 0.01 *dt)

                _recoveryTime = _recoveryTime - dt
                if (_recoveryTime <= 0) {
                    System.print("Recovered")
                    _recoveryState = false
                    _evaluateState = true
                }
            }

            if (_movingState) {
                this.DoPathfinding(playerPos, engine, dt)
                _evaluateState = true   
            }

            if (_evaluateState) {
                if (Math.Distance(playerPos, pos) < _attackRange) {
                    System.print("Enter Attack State")
                    // Enter attack state
                    _attackingState = true
                    _movingState = false
                    body.SetFriction(12.0)
                    animations.Play("Attack", 1.0, false, 1.0, false)
                    animations.SetTime(0.0)
                    _attackTime = _attackMaxTime
                    _evaluateState = false
                    _rootEntity.GetAudioEmitterComponent().AddSFX(engine.GetAudio().PlaySFX("assets/sounds/demon_roar.wav", 1.0))

                } else if (_movingState == false) { // Enter attack state
                    System.print("Enter Moving State")
                    body.SetFriction(0.0)
                    animations.Play("Run", 1.0, true, 0.5, true)
                    _movingState = true
                }
            }    
        } else {
            _deathTimer = _deathTimer - dt

            if (_deathTimer <= 0) {
                engine.GetECS().DestroyEntity(_rootEntity) // Destroys the entity, and in turn this object
            } else {
                var newPos = pos - Vec3.new(1, 1, 1).mulScalar(1.0 * 0.001 * dt)
                body.SetTranslation(newPos)
            }
        }
    }

    DoPathfinding(playerPos, engine, dt) {
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()

        _reasonTimer = _reasonTimer + dt
        if(_reasonTimer > 800) {
            _reasonTimer = 0
            this.FindNewPath(engine)
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
            var startRotation = _rootEntity.GetTransformComponent().rotation
            _rootEntity.GetTransformComponent().rotation = Math.Slerp(startRotation, endRotation, 0.01 *dt)
        }else{
            this.FindNewPath(engine)
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