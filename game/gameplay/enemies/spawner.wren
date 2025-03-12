import "engine_api.wren" for Vec3, Random, Engine
import "enemies.wren" for MeleeEnemy

class Spawner {

    construct new(position, millisecondInterval) {
        _rangeMin = position
        _rangeMax = Vec3.new(-41.8, 19.0, 269.6)
    }

    SpawnEnemies(engine, enemyList, size, maxSpeed, enemyModel, enemyShape, count) {

        for(i in 0...count) {
            var enemy = enemyList.add(MeleeEnemy.new(engine, _rangeMin, size, maxSpeed, enemyModel, enemyShape))
            enemy.FindNewPath(engine)
        }
    }
}