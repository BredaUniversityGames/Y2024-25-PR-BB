import "engine_api.wren" for Vec3, Random, Engine
import "enemies.wren" for MeleeEnemy

class Spawner {

    construct new(engine) {
        if(__engine == null) {
            __engine = engine
        }
        _rangeMin = Vec3.new(-43.8, 19.0, 265.6)
        _rangeMax = Vec3.new(-41.8, 19.0, 269.6)
        _meleeEnemies = [null]
        _meleeEnemies.clear()
    }

    SpawnEnemies(count) {
        if(count == 0) {
            return
        }

        for(i in 0...count) {
            var enemy = _meleeEnemies.add(MeleeEnemy.new(Vec3.new(-42.8, 19.3, 267.6), __engine))
             //var enemy = _meleeEnemies.add(MeleeEnemy.new(Random.RandomVec3VectorRange(_rangeMin, _rangeMax), __engine))
             enemy.FindNewPath()
        }
    }

    Update() {
        for(enemy in _meleeEnemies) {
            enemy.Update()
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