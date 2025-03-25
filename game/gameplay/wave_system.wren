import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, Random

class Spawn {
    construct new(enemyType, spawnLocationId, time, count) {
        _enemyType = enemyType
        _spawnLocationId = spawnLocationId
        _time = time
        _count = count
        _processed = false
    }

    EnemyType{ _enemyType }
    SpawnLocationId { _spawnLocationId }
    Time{ _time }
    Count{ _count }
    Processed{ _processed }
    Processed=(value){ _processed = value }
}

class WaveConfig {
    construct new() {
        _duration = 0.0
        _spawns = []
    }

    Duration{ _duration }
    Spawns{ _spawns }

    SetDuration(duration) {
        _duration = duration
        return this
    }

    AddSpawn(enemyType, spawnLocationId, time, count) {
        _spawns.add(Spawn.new(enemyType, spawnLocationId, time, count))
        return this
    }
}

class WaveStatusType {
    static NotStarted { 0 }
    static Ongoing { 1 } 
    static Completed { 2 }
    static Finished { 3 }
}

class WaveSystem {

    construct new(engine, waveConfigs) {
        _engine = engine
        _waveConfigs = waveConfigs

        _currentWave = -1
        _status = WaveStatusType.NotStarted
        _waveTimer = 0.0
        _spawnedEnemies = []
    }
    
    ActiveWaveConfig() { 
        if(_currentWave >= 0 && _currentWave < _waveConfigs.count) {
            return _waveConfigs[_currentWave]
        }

        return null
    }

    Update(dt) {
        _waveTimer = _waveTimer + dt / 1000.0

        if(_status == WaveStatusType.Ongoing) {

            var allProcessed = true

            for(spawn in this.ActiveWaveConfig().Spawns) {

                if(allProcessed) {
                    allProcessed = spawn.Processed
                }

                if(!spawn.Processed && spawn.Time < _waveTimer) {
                    // TODO: Spawn enemy at location
                    System.print("Spawned %(spawn.Count) enemies")

                    spawn.Processed = true
                }
                
            }

            if(_spawnedEnemies.count == 0 && _waveTimer > this.ActiveWaveConfig().Duration) {
                _status = WaveStatusType.Completed
                _waveTimer = 0.0
            }

        } else if(_status == WaveStatusType.Completed || _status == WaveStatusType.NotStarted) {

            if(_waveTimer > 3.0) {

                if(_currentWave + 1 < _waveConfigs.count) {
                    this.StartNextWave()
                } else if (_status != WaveStatusType.Finished) {
                    _status = WaveStatusType.Finished
                    System.print("Completed all waves")
                }
            
            }
        }
    }

    StartNextWave() {
        if(_status == WaveStatusType.Ongoing) {
            return
        }

        _currentWave = _currentWave + 1
        _status = WaveStatusType.Ongoing
        _waveTimer = 0.0
            
        System.print("Starting wave %(_currentWave)")
    }
}
