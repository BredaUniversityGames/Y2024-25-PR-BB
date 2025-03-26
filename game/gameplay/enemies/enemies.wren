import "engine_api.wren" for Vec3, Engine, ShapeFactory, Rigidbody, RigidbodyComponent, CollisionShape, Math

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

        _reasonTimer = 0.0
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

    Update(playerPos, engine, dt) {
        
        var body = _rootEntity.GetRigidbodyComponent()
        var pos = body.GetPosition()
        _rootEntity.GetTransformComponent().translation = pos

        _reasonTimer = _reasonTimer + dt
        if(_reasonTimer > 300) {
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


            // --------
            //  WIP local avoidance
            // --------

            // var rayHitInfos = engine.GetPhysics().ShootMultipleRays(Vec3.new(pos.x,pos.y - 1.3,pos.z),Vec3.new(forwardVector.x, 0.4, forwardVector.z), 6.0, 5,20)

            // var averageNormal = Vec3.new(0,0,0)
            // var amountOfHits = 0
            // for( rayHit in rayHitInfos) {

            //      if(rayHit.GetEntity(engine.GetECS()).GetNameComponent().name != _rootEntity.GetNameComponent().name && rayHit.GetEntity(engine.GetECS()) != null){
            //         var normal = rayHit.normal
            //         normal.y = 0.0
            //         averageNormal = averageNormal + normal
            //         amountOfHits = amountOfHits + 1

            //         //System.print("Hit")
            //         System.print(rayHit.GetEntity(engine.GetECS()).GetNameComponent().name)
            //         //System.print(_rootEntity.GetNameComponent().name)
                    
            //         //System.print(rayHit.GetEntity(engine.GetECS()).GetNameComponent().name == _rootEntity.GetNameComponent().name )

            //     }
            // }

            // if(amountOfHits > 0 ){
            //     averageNormal.x = averageNormal.x / amountOfHits
            //     averageNormal.y = averageNormal.y / amountOfHits
            //     averageNormal.z = averageNormal.z / amountOfHits
            // }
            // forwardVector = Math.MixVec3(forwardVector, forwardVector + averageNormal, 0.1*dt)   

            // --------
            //  WIP local avoidance
            // --------

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