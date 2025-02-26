import "engine_api.wren" for Vec3, Engine

class MeleeEnemy {

    construct new(spawnPosition, engine) {
        __rootEntity = engine.LoadModelIntoECS("assets/models/demon.glb")[0]
        __rootEntity.GetTransformComponent().translation = spawnPosition
        __rootEntity.GetTransformComponent().scale = Vec3.new(0.01, 0.01, 0.01)
    }

    GetPosition() {
        return __rootEntity.GetTransformComponent().translation
    }

    SetPosition(newPos) {
        __rootEntity.GetTransformComponent().translation = newPos
    }

    Update(engine) {
        
    }

}