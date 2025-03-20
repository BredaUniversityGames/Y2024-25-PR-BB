import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, Random

class WaveSystem {

    construct new(spawnPoints, enemyModels) {
        _spawnPoints = spawnPoints
        _enemyModels = enemyModels

        _timer = 0.0

        _waveDelay = 5000.0
        _waveDelayMult = 1.01

        _waveCount = 1.0
        _waveCountAdd = 0.07
    }

    SpawnEnemy(engine, playerPosition) {

        // Pick random spawnpoint

        var index = Random.RandomIndex(0, _spawnPoints.count)
        var spawnPos = _spawnPoints[index]

        // Pick random model

        var index2 = Random.RandomIndex(0, _enemyModels.count)
        var enemyModel = _enemyModels[index2]

        // Spawn entity

        var entity = engine.LoadModel(enemyModel)

        entity.GetTransformComponent().translation = spawnPos
        // entity.GetTransformComponent().rotation = Quat.
        entity.GetTransformComponent().scale = Vec3.new(0.025, 0.025, 0.025)

        var demonAnimations = entity.GetAnimationControlComponent()
        demonAnimations.Play("Idle", 1.0, true, 0.0, false)
    }
    
    Update(engine, playerPosition, dt) {

        _timer = _timer + dt
        var enemiesToSpawn = 0

        if (_timer > _waveDelay) {
            _waveDelay = _waveDelay / _waveDelayMult
            _timer = _timer % _waveDelay

            enemiesToSpawn = _waveCount
            _waveCount = _waveCount + _waveCountAdd
        }

        for (i in 0...enemiesToSpawn) {
            this.SpawnEnemy(engine, playerPosition)
        }
    }
}
