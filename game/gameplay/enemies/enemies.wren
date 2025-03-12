import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Math

class MeleeEnemy {

    construct new(engine, spawnPosition, size, maxSpeed, enemyModel, colliderShape) {
        
        _maxVelocity = maxSpeed
        _currentPath = null
        _currentPathNodeIdx = null

        _velocityDirection = Vec3.new(_maxVelocity, 0, 0)

        _rootEntity = engine.LoadModelIntoECS(enemyModel)[0]
        _rootEntity.GetTransformComponent().translation = spawnPosition
        _rootEntity.GetTransformComponent().scale = size

        var rb = Rigidbody.new(engine.GetPhysics(), colliderShape, true, false)
        var body = _rootEntity.AddRigidbodyComponent(rb)
        body.SetFriction(0)

        var animations = _rootEntity.GetAnimationControlComponent()
        animations.Play("Run", 1.0, true)

    }

    entity {
        return _rootEntity
    }

    position {
        return _rootEntity.GetTransformComponent().translation
    }

    position=(newPos) {
        _rootEntity.GetTransformComponent().translation = newPos
    }

    Update(dt) {

        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()
        _rootEntity.GetTransformComponent().translation = pos

        // Pathfinding logic
        if(_currentPath != null) {
            
            var waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]

            if((waypoint.center - pos).length() < 0.3) {
                _currentPathNodeIdx = _currentPathNodeIdx + 1
                if(_currentPathNodeIdx == _currentPath.GetWaypoints().count) {
                    body.SetVelocity(Vec3.new(0.0, 0.0, 0.0))
                    _currentPath = null
                    return
                }
                waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]
            }

            var forwardVector = (waypoint.center - pos).normalize()
            var velocity = _maxVelocity * dt
            System.print(velocity)

            _rootEntity.GetRigidbodyComponent().SetVelocity(forwardVector.mulScalar(velocity))

            // Set forward rotation
            _rootEntity.GetTransformComponent().rotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))
        }
    }

    FindNewPath(engine) {
        var body = _rootEntity.GetRigidbodyComponent()
        var startPos = body.GetPosition()
        _currentPath = engine.GetPathfinding().FindPath(startPos, Vec3.new(-16.0, 29.0, 195.1))
        _currentPathNodeIdx = 0

        var waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]
        var velocity = (waypoint.center - startPos).normalize() * Vec3.new(_maxVelocity, _maxVelocity, _maxVelocity)
        body.SetVelocity(velocity)
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