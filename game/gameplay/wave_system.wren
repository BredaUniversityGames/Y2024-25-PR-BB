import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, Random, ShapeFactory
import "enemies/enemies.wren" for MeleeEnemy

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

    construct new(engine, waveConfigs, enemyList) {
        _engine = engine
        _waveConfigs = waveConfigs
        _enemyList = enemyList

        _currentWave = -1
        _status = WaveStatusType.NotStarted
        _waveTimer = 0.0
        _enemyShape = ShapeFactory.MakeCapsuleShape(70.0, 70.0)
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
                    this.Spawn(spawn)

                    spawn.Processed = true
                }
                
            }

            if(_enemyList.count == 0 && _waveTimer > this.ActiveWaveConfig().Duration) {
                _status = WaveStatusType.Completed
                _waveTimer = 0.0

                System.print("Completed wave")
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

    Spawn(spawn) {
        var enemyModelPath = "assets/models/Skeleton.glb"
        var enemy = _enemyList.add(MeleeEnemy.new(_engine, Vec3.new(10.0, 14.4, 11.4), Vec3.new(0.02, 0.02, 0.02), 5, enemyModelPath, _enemyShape))
        enemy.FindNewPath(_engine)
    }
}
