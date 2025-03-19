import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Math

class MeleeEnemy {

    construct new(engine, spawnPosition, size, maxSpeed, enemyModel, colliderShape, UID) {
        
        _maxVelocity = maxSpeed
        _currentPath = null
        _currentPathNodeIdx = null

        _velocityDirection = Vec3.new(_maxVelocity, 0, 0)

        _enemyUID = UID
        
        _meshEntity = engine.LoadModel(enemyModel)

        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "Enemy"
        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition
        transform.scale = size

        _rootEntity.AttachChild(_meshEntity)
        _meshEntity.GetTransformComponent().translation = Vec3.new(0,-27,0)

        var rb = Rigidbody.new(engine.GetPhysics(), colliderShape, true, false)
        var body = _rootEntity.AddRigidbodyComponent(rb)
        // body.SetFriction(2.0)

        var animations = _meshEntity.GetAnimationControlComponent()
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

    Update(playerPos, dt) {

        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()
        _rootEntity.GetTransformComponent().translation = pos

        var forwardVector = (playerPos - pos).normalize()
        var velocity = forwardVector.mulScalar(_maxVelocity)//_rootEntity.GetRigidbodyComponent().GetVelocity() + forwardVector.mulScalar(_maxVelocity * 1000 / dt)

        _rootEntity.GetRigidbodyComponent().SetVelocity(velocity)
        _rootEntity.GetTransformComponent().rotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))

        // Pathfinding is unused due to lack of navmesh for blockout

        // Pathfinding logic
        // if(_currentPath != null) {
            
        //     var waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]

        //     if((waypoint.center - pos).length() < 0.3) {
        //         _currentPathNodeIdx = _currentPathNodeIdx + 1
        //         if(_currentPathNodeIdx == _currentPath.GetWaypoints().count) {
        //             body.SetVelocity(Vec3.new(0.0, 0.0, 0.0))
        //             _currentPath = null
        //             return
        //         }
        //         waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]
        //     }

        //     var forwardVector = (waypoint.center - pos).normalize()
        //     var velocity = _maxVelocity * dt
        //     System.print(velocity)

        //     _rootEntity.GetRigidbodyComponent().SetVelocity(forwardVector.mulScalar(velocity))

        //     // Set forward rotation
        //     _rootEntity.GetTransformComponent().rotation = Math.LookAt(Vec3.new(forwardVector.x, 0, forwardVector.z), Vec3.new(0, 1, 0))
        // }
    }

    FindNewPath(engine) {
        var startPos = position
        _currentPath = engine.GetPathfinding().FindPath(startPos, Vec3.new(-16.0, 29.0, 195.1))
        _currentPathNodeIdx = 0
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