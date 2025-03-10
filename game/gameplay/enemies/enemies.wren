import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Math

class MeleeEnemy {

    construct new(spawnPosition, engine) {
        if(__engine == null) {
            __engine = engine
        }

        if(__physicsShape == null) {
            __physicsShape = ShapeFactory.MakeCapsuleShape(1.7, 0.5)
        }

        __maxVelocity = 5.8
        _currentPath = null
        _currentPathNodeIdx = null

        _velocityDirection = Vec3.new(__maxVelocity, 0, 0)
        this.Create(spawnPosition)
    }

    GetPosition() {
        return _rootEntity.GetTransformComponent().translation
    }

    SetPosition(newPos) {
        _rootEntity.GetTransformComponent().translation = newPos
    }

    Update() {
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
            var velocity = forwardVector * Vec3.new(__maxVelocity, __maxVelocity, __maxVelocity)
            body.SetVelocity(velocity)

            // Set forward rotation
            _rootEntity.GetTransformComponent().rotation = Math.ToQuat(Vec3.new(forwardVector.y, 0, forwardVector.z))
        }
    }

    IsValid() {
        return _rootEntity != null
    }

    FindNewPath() {
        var body = _rootEntity.GetRigidbodyComponent()
        var startPos = body.GetPosition()
        _currentPath = __engine.GetPathfinding().FindPath(startPos, Vec3.new(-16.0, 29.0, 195.1))
        _currentPathNodeIdx = 0

        var waypoint = _currentPath.GetWaypoints()[_currentPathNodeIdx]
        var velocity = (waypoint.center - startPos).normalize() * Vec3.new(__maxVelocity, __maxVelocity, __maxVelocity)
        body.SetVelocity(velocity)
    }

    Create(spawnPosition) {
        _rootEntity = __engine.LoadModelIntoECS("assets/models/demon.glb")[0]
        _rootEntity.GetTransformComponent().translation = spawnPosition
        _rootEntity.GetTransformComponent().scale = Vec3.new(0.01, 0.01, 0.01)

        var rb = Rigidbody.new(__engine.GetPhysics(), __physicsShape, true, false)
        var body = _rootEntity.AddRigidbodyComponent(rb)
        body.SetFriction(0)
    }

    Destroy() {
        if(!this.IsValid()) {
            return
        }

        var ecs = __engine.GetECS()
        ecs.DestroyEntity(_rootEntity)
        _rootEntity = null
    }

}