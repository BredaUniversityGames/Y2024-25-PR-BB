import "engine_api.wren" for Vec3, Engine

class MeleeEnemy {

    construct new(spawnPosition, engine) {
        if(__engine == null) {
            __engine = engine
        }
        this.Create(spawnPosition)
    }

    GetPosition() {
        return _rootEntity.GetTransformComponent().translation
    }

    SetPosition(newPos) {
        _rootEntity.GetTransformComponent().translation = newPos
    }

    Update(engine) {

    }

    IsValid() {
        return _rootEntity != null
    }

    Create(spawnPosition) {
        _rootEntity = __engine.LoadModelIntoECS("assets/models/demon.glb")[0]
        _rootEntity.GetTransformComponent().translation = spawnPosition
        _rootEntity.GetTransformComponent().scale = Vec3.new(0.01, 0.01, 0.01)
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