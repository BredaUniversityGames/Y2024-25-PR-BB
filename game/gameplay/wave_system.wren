import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, Random, ShapeFactory
import "enemies/melee.wren" for MeleeEnemy
import "enemies/ranged.wren" for RangedEnemy
import "enemies/berserker.wren" for BerserkerEnemy
import "station.wren" for PowerUpType, Station, StationManager

class EnemyType {
    static Skeleton { "Skeleton" }
    static Eye { "Eye" }
    static Berserker { "Berserker" }
}

class WaveConfig {
    construct new() {

        _spawns = {
            EnemyType.Skeleton: 0,
            EnemyType.Eye: 0,
            EnemyType.Berserker: 0
        }
    }

    Spawns{ _spawns }

    SetEnemySpawnCount(enemyType, count) {
        _spawns[enemyType] = count
        return this
    }
}

class WaveGenerator {

    static GenerateWave(waveNumber) {

        var wave = WaveConfig.new()
        
        var difficultyScore = (waveNumber + 2) * (waveNumber + 2) + 20
        var heavyEnemyBias = Math.Clamp(waveNumber / 30, 0.0, 1.0)

        var skeletonPoints = Math.Floor(difficultyScore * (0.20 + (1.0 - heavyEnemyBias) * 0.55))
        var eyePoints = Math.Floor(difficultyScore * (0.15 + heavyEnemyBias * 0.05))
        var berserkerPoints = Math.Floor(difficultyScore * (0.10 + heavyEnemyBias * 0.40))

        var skeletonCount = Math.Floor(skeletonPoints / 3)
        var eyeCount = Math.Floor(eyePoints / 4)
        var berserkerCount = Math.Floor(berserkerPoints / 7)

        wave.SetEnemySpawnCount(EnemyType.Skeleton, skeletonCount)
            .SetEnemySpawnCount(EnemyType.Eye, eyeCount)
            .SetEnemySpawnCount(EnemyType.Berserker, berserkerCount)

        //System.print("Points %(difficultyScore) / %(heavyEnemyBias) - Skeletons: %(skeletonPoints), Eyes: %(eyePoints), Berserkers: %(berserkerPoints)")
        System.print("Wave %(waveNumber) - Skeletons: %(skeletonCount), Eyes: %(eyeCount), Berserkers: %(berserkerCount)")
        return wave
    }
}

class WaveSystem {

    construct new(engine, waveConfigs, spawnLocations) {
    
        _waveConfigs = waveConfigs
        _spawnLocations = spawnLocations
        _ongoingWave = false
        _currentWave = -1
        _waveTimer = 0.0
        _waveDelay = 4.0
        _waveSlowdown = 0.25
        _enemyCount = 0

        _timeSpeed = 1.0
        _slowTime = 0.1

        var entity = engine.GetECS().NewEntity()
        entity.AddTransformComponent().translation = Vec3.new(-80, 111, 44)
        _audioEmitter = entity.AddAudioEmitterComponent()

        if(_spawnLocations.count == 0) {
            System.print("Should pass at least one spawn location to the wave system!")
        }
    }


    Update(engine, player, enemyList, dt, playerVariables, stationManager) {

        _waveTimer = _waveTimer + dt / 1000.0

        // Finished all waves
        if (_currentWave >= _waveConfigs.count) {

            if (_currentWave == _waveConfigs.count) {
                _currentWave = _currentWave + 1
                System.print("Completed all waves!")
            }
            return
        }

        var realDeltatime = engine.GetTime().GetRealDeltatime() / 1000 * 60

        // Waiting period
        if (_waveTimer < _waveDelay) {
            if (_waveTimer > _waveSlowdown) {
                _timeSpeed = Math.MixFloat(_timeSpeed, 1.0, 0.12 * realDeltatime)
            } else {
                if (_currentWave > -1) {
                    _timeSpeed = Math.MixFloat(_timeSpeed, _slowTime, 0.10 * realDeltatime)
                }
            }
            engine.GetTime().SetScale(_timeSpeed)
            engine.GetAudio().SetPlaybackSpeed(_timeSpeed)
            return
        }

        _timeSpeed = Math.MixFloat(_timeSpeed, 1.0, 0.12  * realDeltatime)

        engine.GetTime().SetScale(_timeSpeed)
        engine.GetAudio().SetPlaybackSpeed(_timeSpeed)

        // Inside a wave
        if (_ongoingWave) {

            if (_enemyCount == 0) {

                _ongoingWave = false
                _waveTimer = 0.0
                System.print("Completed wave %(_currentWave)")
            }

        } else {
          
            this.NextWave(engine, enemyList)
            stationManager.ResetStations()
            playerVariables.hud.IncrementWaveCounter(_currentWave)
        }
    }

    SpawnWave(engine, enemyList, wave) {

        // Spawn skeletons
        for (v in 0...wave.Spawns[EnemyType.Skeleton]) {
            var enemy = enemyList.add(MeleeEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 1, 0), _currentWave))
            enemy.FindNewPath(engine)
            _enemyCount = _enemyCount + 1
        }

        // Spawn eyes
        for (v in 0...wave.Spawns[EnemyType.Eye]) {
            var enemy = enemyList.add(RangedEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 6, 0)))
            enemy.FindNewPath(engine)
            _enemyCount = _enemyCount + 1
        }

        // Spawn berserkers
        for (v in 0...wave.Spawns[EnemyType.Berserker]) {
            var enemy = enemyList.add(BerserkerEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 3, 0), _currentWave))
            enemy.FindNewPath(engine)
            _enemyCount = _enemyCount + 1
        }
    }

    NextWave(engine, enemyList) {

        // Start the next wave
        _currentWave = _currentWave + 1

        var eventInstance = engine.GetAudio().PlayEventOnce("event:/SFX/WaveStart")
        engine.GetAudio().SetEventVolume(eventInstance, 1.5)
        _audioEmitter.AddEvent(eventInstance)

        if (_currentWave < _waveConfigs.count) {
            _enemyCount = 0
            var activeWave = _waveConfigs[_currentWave]
            this.SpawnWave(engine, enemyList, activeWave)

            System.print("Starting wave %(_currentWave + 1)")
        }

        _ongoingWave = true
    }

    GetSpawnLocation() {
        return _spawnLocations[Random.RandomIndex(0, _spawnLocations.count - 1)]
    }

    DecreaseEnemyCount() {
        _enemyCount = _enemyCount - 1
    }
}
