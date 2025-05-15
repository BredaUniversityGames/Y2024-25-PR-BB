import "engine_api.wren" for Vec3, Random, Engine
import "enemies.wren" for MeleeEnemy
import "berserker_enemy.wren" for BerserkerEnemy

class Spawner {

    construct new(position, millisecondInterval) {
        _rangeMin = position
        _interval = millisecondInterval
        _timer = 0
    }

    SpawnEnemies(engine, enemyList, size, maxSpeed, enemyModel, enemyShape, count) {

        for(i in 0...count) {
            //System.print("Spawned an Enemy")
            var enemy = enemyList.add(MeleeEnemy.new(engine, _rangeMin, size, maxSpeed, enemyModel, enemyShape))
            enemy.FindNewPath(engine)
        }
    }

    Update(engine, enemyList, size, maxSpeed, enemyModel, enemyShape, dt) {
        _timer = _timer + dt
        
        if (_timer > _interval) {
            this.SpawnEnemies(engine, enemyList, size, maxSpeed, enemyModel, enemyShape, 1)
            _timer = 0
        }
    }
}