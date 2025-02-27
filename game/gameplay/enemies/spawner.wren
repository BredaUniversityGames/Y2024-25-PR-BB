import "engine_api.wren" for Vec3, Random, Engine
import "enemies.wren" for MeleeEnemy

class Spawner {

    construct new(engine) {
        if(__engine == null) {
            __engine = engine
        }
        _rangeMin = Vec3.new(-44.8, 19.0, 264.6)
        _rangeMax = Vec3.new(-40.8, 19.0, 270.6)
        _meleeEnemies = [null]
        _meleeEnemies.clear()
    }

    SpawnEnemies(count) {
        if(count == 0) {
            return
        }

        for(i in 0...count) {
             _meleeEnemies.add(MeleeEnemy.new(Random.RandomVec3VectorRange(_rangeMin, _rangeMax), __engine))
        }
    }

    ClearAllEnemies() {
        for(enemy in _meleeEnemies) {
            enemy.Destroy()
        }
    }

    Destroy() {
        this.ClearAllEnemies()
        __engine = null
    }

}