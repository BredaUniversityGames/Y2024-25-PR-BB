import "engine_api.wren" for Engine, TracyZone, Math, Random, Stats, Vec3

import "enemies/melee.wren" for MeleeEnemy
import "enemies/ranged.wren" for RangedEnemy
import "enemies/berserker.wren" for BerserkerEnemy
import "station.wren" for PowerUpType, Station, StationManager

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

    construct new(engine, waveConfigs, spawnLocations) {
    
        _waveIndex = -1
        _waveConfigs = waveConfigs
        _spawnLocations = spawnLocations

        _ongoingWave = false

        _currentWave = Wave.new()
        _waveDelay = 6.0
        _waveSlowdown = 0.8
        _realWaveTimer = _waveSlowdown * 2
        _enemyCount = 0

        _timeSpeed = 1.0
        _slowTime = 0.15

        var entity = engine.GetECS().NewEntity()
        entity.AddTransformComponent().translation = Vec3.new(-80, 111, 44)
        _audioEmitter = entity.AddAudioEmitterComponent()

        _waveSpawnTimer = 0.0
        _waveSpawnInterval = 0.2

        if(_spawnLocations.count == 0) {
            System.print("Should pass at least one spawn location to the wave system!")
        }
    }


    Update(engine, player, enemyList, dt, playerVariables, stationManager) {

        _realWaveTimer = _realWaveTimer + engine.GetTime().GetRealDeltatime() / 1000.0
        _waveSpawnTimer = _waveSpawnTimer + dt / 1000.0

        var realDeltatime = engine.GetTime().GetRealDeltatime() / 1000 * 60

        // Waiting period
        if (_realWaveTimer < _waveDelay) {

            if (_realWaveTimer < _waveSlowdown) {
                _timeSpeed = Math.MixFloat(_timeSpeed, _slowTime, _realWaveTimer / 0.8)
            } else if (_realWaveTimer < _waveSlowdown * 2) {
                _timeSpeed = Math.MixFloat(_slowTime, 1.0, (_realWaveTimer - _waveSlowdown) / 0.8)
            } 

            engine.GetTime().SetScale(_timeSpeed)
            engine.GetAudio().SetPlaybackSpeed(_timeSpeed)
            return
        }

        // Inside a wave
        if (_ongoingWave) {

            if (_waveSpawnTimer > _waveSpawnInterval) {
                this.SpawnEnemy(engine, enemyList)
                _waveSpawnTimer = 0.0
            }

            if (_enemyCount == 0 && _currentWave.isOver()) {

                _ongoingWave = false
                _realWaveTimer = 0.0
                System.print("Completed wave %(_waveIndex)")
            }

        } else {
          
            this.NextWave(engine, playerVariables)
            stationManager.ResetStations()

            var playerPowerUp = playerVariables.GetCurrentPowerUp()
            if(playerVariables.GetCurrentPowerUp() == PowerUpType.NONE){
                stationManager.timer = stationManager.timer + stationManager.intervalBetweenStations
            }
        }
    }

    SpawnEnemy(engine, enemyList) {

        var zone = TracyZone.new("Enemy Spawn")

        engine.GetTime().SetScale(1.0)
        engine.GetAudio().SetPlaybackSpeed(1.0)

        var enemyType = EnemyType.Skeleton

        if (enemyType == EnemyType.Skeleton) {

            if (_currentWave.spawns[enemyType] > 0) {
                var enemy = enemyList.add(MeleeEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 1, 0)))
                _currentWave.spawns[enemyType] = _currentWave.spawns[enemyType] - 1
                enemy.FindNewPath(engine)
                _enemyCount = _enemyCount + 1
            } else {
                enemyType = EnemyType.Eye
            }

        } else if (enemyType == EnemyType.Eye) {

            if (_currentWave.spawns[enemyType] > 0) {
                var enemy = enemyList.add(RangedEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(0, 6, 0)))
                _currentWave.spawns[enemyType] = _currentWave.spawns[enemyType] - 1
                enemy.FindNewPath(engine)
                _enemyCount = _enemyCount + 1
            } else {
                enemyType = EnemyType.Berserker
            }

        } else if (enemyType == EnemyType.Berserker) {

            if (_currentWave.spawns[enemyType] > 0) {
                var offsetX = Random.RandomFloatRange(-1, 1)
                var offsetZ = Random.RandomFloatRange(-1, 1)
                var enemy = enemyList.add(BerserkerEnemy.new(engine, this.GetSpawnLocation() + Vec3.new(offsetX, 3, offsetZ)))
                _currentWave.spawns[enemyType] = _currentWave.spawns[enemyType] - 1
                enemy.FindNewPath(engine)
                _enemyCount = _enemyCount + 1
            }
        }

        zone.End()
    }

    NextWave(engine, playerVariables) {

        // Start the next wave
        _waveIndex = _waveIndex + 1

        if (_waveIndex == _waveConfigs.count) {
            _waveConfigs.add(WaveGenerator.GenerateWave(_waveIndex))
        }

        var eventInstance = engine.GetAudio().PlayEventOnce("event:/SFX/WaveStart")
        _audioEmitter.AddEvent(eventInstance)

        var stat = engine.GetSteam().GetStat(Stats.WAVES_REACHED())
        if(stat != null) {
            if(_waveIndex > stat.intValue) {
                stat.intValue = _waveIndex
            }
        }

        _currentWave = _waveConfigs[_waveIndex]
        System.print("Starting wave %(_waveIndex + 1)")

        _enemyCount = 0

        _ongoingWave = true
        playerVariables.hud.IncrementWaveCounter(_waveIndex)
    }

    GetSpawnLocation() {
        return _spawnLocations[Random.RandomIndex(0, _spawnLocations.count - 1)]
    }

    DecreaseEnemyCount() {
        _enemyCount = _enemyCount - 1
    }
}
