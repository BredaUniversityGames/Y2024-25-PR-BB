import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, Random, ShapeFactory
import "enemies/melee.wren" for MeleeEnemy
import "enemies/ranged.wren" for RangedEnemy
import "enemies/berserker.wren" for BerserkerEnemy

class EnemyType {
    static Skeleton { "Skeleton" }
    static Eye { "Eye" }
    static Berserker { "Berserker" }
}

class Wave {
    construct new() {

        _spawns = {
            EnemyType.Skeleton: 0,
            EnemyType.Eye: 0,
            EnemyType.Berserker: 0
        }
    }

    isOver() {
        return _spawns[EnemyType.Eye] == 0 && _spawns[EnemyType.Skeleton] == 0 && _spawns[EnemyType.Berserker] == 0
    }

    spawns { _spawns }
}

class WaveGenerator {

    static GenerateWave(waveNumber) {

        var wave = Wave.new()
        
        var difficultyScore = (waveNumber + 2) * (waveNumber + 2) + 20
        var heavyEnemyBias = Math.Clamp(waveNumber / 30, 0.0, 1.0)

        var skeletonPoints = Math.Floor(difficultyScore * (0.20 + (1.0 - heavyEnemyBias) * 0.55))
        var eyePoints = Math.Floor(difficultyScore * (0.15 + heavyEnemyBias * 0.05))
        var berserkerPoints = Math.Floor(difficultyScore * (0.10 + heavyEnemyBias * 0.40))

        var skeletonCount = Math.Floor(skeletonPoints / 3)
        var eyeCount = Math.Floor(eyePoints / 4)
        var berserkerCount = Math.Floor(berserkerPoints / 7)

        wave.spawns[EnemyType.Skeleton] = skeletonCount
        wave.spawns[EnemyType.Eye] = eyeCount
        wave.spawns[EnemyType.Berserker] = berserkerCount

        System.print("Wave %(waveNumber) - Skeletons: %(skeletonCount), Eyes: %(eyeCount), Berserkers: %(berserkerCount)")
        return wave
    }
}

class WaveSystem {

    construct new(waveConfigs, spawnLocations) {
    
        _waveIndex = -1
        _waveConfigs = waveConfigs
        _spawnLocations = spawnLocations

        _ongoingWave = false

        _currentWave = Wave.new()
        _waveTimer = 0.0
        _waveDelay = 4.0

        _waveSpawnTimer = 0.0
        _waveSpawnInterval = 0.2

        if(_spawnLocations.count == 0) {
            System.print("Should pass at least one spawn location to the wave system!")
        }
    }

    Update(engine, player, enemyList, dt, playerVariables) {

        _waveTimer = _waveTimer + dt / 1000.0
        _waveSpawnTimer = _waveSpawnTimer + dt / 1000.0

        // Waiting period
        if (_waveTimer < _waveDelay) {
            return
        }

        // Inside a wave
        if (_ongoingWave) {

            if (_waveSpawnTimer > _waveSpawnInterval) {
                this.SpawnEnemy(engine, enemyList)
                _waveSpawnTimer = 0.0
            }

            if (enemyList.count == 0 && _currentWave.isOver()) {

                _ongoingWave = false
                _waveTimer = 0.0

                System.print("Completed wave %(_waveIndex)")
            }

        } else {
          
            this.NextWave(engine)
            playerVariables.hud.IncrementWaveCounter(_waveIndex)
        }
    }

    SpawnEnemy(engine, enemyList) {
        // Spawn skeletons
        for (v in 0..._currentWave.spawns[EnemyType.Skeleton]) {
            var enemy = enemyList.add(MeleeEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 1, 0)))
            enemy.FindNewPath(engine)
        }

        // Spawn eyes
        for (v in 0..._currentWave.spawns[EnemyType.Eye]) {
            var enemy = enemyList.add(RangedEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 6, 0)))
            enemy.FindNewPath(engine)
        }

        // Spawn berserkers
        for (v in 0..._currentWave.spawns[EnemyType.Berserker]) {
            var enemy = enemyList.add(BerserkerEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 3, 0)))
            enemy.FindNewPath(engine)
        }

        System.print("Spawning wave %(_waveIndex + 1)")
        _currentWave = Wave.new()
    }

    NextWave(engine) {

        // Start the next wave
        _waveIndex = _waveIndex + 1

        if (_waveIndex < _waveConfigs.count) {
            _currentWave = _waveConfigs[_waveIndex]
            System.print("Starting wave %(_waveIndex + 1)")
        }

        _ongoingWave = true
    }

    GetSpawnLocation() {
        return _spawnLocations[Random.RandomIndex(0, _spawnLocations.count - 1)]
    }
}
