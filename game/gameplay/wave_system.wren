import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, Random, ShapeFactory
import "enemies/melee.wren" for MeleeEnemy
import "enemies/ranged.wren" for RangedEnemy
import "enemies/berserker.wren" for BerserkerEnemy

class EnemyType {
    static Skeleton { 0 }
    static Eye { 1 }
    static Berserker { 2 }
    static SIZE { 3 }
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

        var skeletonFormula = (0.8 * waveNumber * waveNumber + 25) * (0.08 + (1 - waveNumber / 30) * 0.1) 
        var eyeFormula = Math.Pow(waveNumber, 1.25) * 0.2
        var berserkerFormula = (0.7 * waveNumber * waveNumber + 50) * (0.005 + (waveNumber / 20) * 0.03)   
     
        wave.spawns[EnemyType.Skeleton] = Math.Floor(skeletonFormula)
        wave.spawns[EnemyType.Eye] = Math.Floor(eyeFormula)
        wave.spawns[EnemyType.Berserker] =  Math.Floor(berserkerFormula)

        System.print("Wave %(waveNumber + 1) - Skeletons: %(wave.spawns[EnemyType.Skeleton]), Eyes: %(wave.spawns[EnemyType.Eye]), Berserkers: %(wave.spawns[EnemyType.Berserker])")
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
        _waveSpawnInterval = 0.1

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
          
            this.NextWave(engine, playerVariables)
        }
    }

    SpawnEnemy(engine, enemyList) {

        var enemyType = Random.RandomIndex(0, EnemyType.SIZE)

        if (enemyType == EnemyType.Skeleton) {

            if (_currentWave.spawns[enemyType] > 0) {
                var enemy = enemyList.add(MeleeEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 1, 0), _waveIndex))
                _currentWave.spawns[enemyType] = _currentWave.spawns[enemyType] - 1
                enemy.FindNewPath(engine)
                return
            } else {
                enemyType = EnemyType.Eye
            }

        } else if (enemyType == EnemyType.Eye) {

            if (_currentWave.spawns[enemyType] > 0) {
                var enemy = enemyList.add(RangedEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 6, 0)))
                _currentWave.spawns[enemyType] = _currentWave.spawns[enemyType] - 1
                enemy.FindNewPath(engine)
                return
            } else {
                enemyType = EnemyType.Berserker
            }

        } else if (enemyType == EnemyType.Berserker) {

            if (_currentWave.spawns[enemyType] > 0) {
                var enemy = enemyList.add(BerserkerEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 3, 0), _waveIndex))
                _currentWave.spawns[enemyType] = _currentWave.spawns[enemyType] - 1
                enemy.FindNewPath(engine)
                return
            }
        }
    }

    NextWave(engine, playerVariables) {

        // Start the next wave
        _waveIndex = _waveIndex + 1

        if (_waveIndex == _waveConfigs.count) {
            _waveConfigs.add(WaveGenerator.GenerateWave(_waveIndex))
        }

        _currentWave = _waveConfigs[_waveIndex]
        System.print("Starting wave %(_waveIndex + 1)")

        _ongoingWave = true
        playerVariables.hud.IncrementWaveCounter(_waveIndex)
    }

    GetSpawnLocation() {
        return _spawnLocations[Random.RandomIndex(0, _spawnLocations.count - 1)]
    }
}
