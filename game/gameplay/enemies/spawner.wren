import "engine_api.wren" for Vec3, Random, Engine
import "enemies.wren" for MeleeEnemy

class Spawner {

    construct new(engine) {
        __engine = engine
        __rangeMin = Vec3.new(-44.8, 19.3, 264.6)
        __rangeMax = Vec3.new(-40.8, 20.1, 270.6)
    }

    SpawnEnemies(count) {
        var startCount = 1
        if(__meleeEnemies == null) {
            __meleeEnemies = [MeleeEnemy.new(Random.RandomVec3VectorRange(__rangeMin, __rangeMax), __engine)]
            startCount = 2
        }

        for(i in startCount..count) {
            __meleeEnemies.add(MeleeEnemy.new(Random.RandomVec3VectorRange(__rangeMin, __rangeMax), __engine))
        }
    }

    GetEnemies() {
        return __meleeEnemies
    }

}