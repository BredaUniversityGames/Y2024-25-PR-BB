import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, Random, ShapeFactory
import "enemies/enemies.wren" for MeleeEnemy
import "enemies/ranged_enemy.wren" for RangedEnemy
import "enemies/berserker_enemy.wren" for BerserkerEnemy


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

class SpawnLocationType {
    static Closest { -1 }
    static Furthest { -2 }
}

class WaveSystem {

    construct new(engine, waveConfigs, enemyList, spawnLocations, player) {
        _engine = engine
        _waveConfigs = waveConfigs
        _enemyList = enemyList
        _spawnLocations = spawnLocations
        _player = player

        _currentWave = -1
        _status = WaveStatusType.NotStarted
        _waveTimer = 0.0
        _enemyShape = ShapeFactory.MakeCapsuleShape(100.0, 35.0)

        if(_spawnLocations.count == 0) {
            System.print("Should pass at least one spawn location to the wave system!")
        }
    }

    WaveDelay { 7.0 }
    
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

            if(_waveTimer > this.WaveDelay) {

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

        if(spawn.EnemyType=="Skeleton"){
            var enemyModelPath = "assets/models/Skeleton.glb"
            if(spawn.SpawnLocationId > _spawnLocations.count) {
                System.error("Invalid spawn location ID %(spawn.SpawnLocationId)")
            }

            var spawnerEntity = this.FindLocation(spawn)
            var position = spawnerEntity.GetTransformComponent().translation

            for(i in 0...spawn.Count) {
                var enemy = _enemyList.add(MeleeEnemy.new(_engine, position, Vec3.new(0.02, 0.02, 0.02), 10, enemyModelPath, _enemyShape))
                enemy.FindNewPath(_engine)
            }
            return
        }

        if(spawn.EnemyType=="Eye"){
            var enemyModelPath = "assets/models/eye.glb"
            var enemyShape = ShapeFactory.MakeSphereShape(0.65)

            if(spawn.SpawnLocationId > _spawnLocations.count) {
                System.error("Invalid spawn location ID %(spawn.SpawnLocationId)")
            }

            var spawnerEntity = this.FindLocation(spawn)
            var position = spawnerEntity.GetTransformComponent().translation
            position.y = position.y + 14.0

            for(i in 0...spawn.Count) {
                var enemy = _enemyList.add(RangedEnemy.new(_engine, position,  Vec3.new(2.25,2.25,2.25), 5, enemyModelPath, enemyShape))
                enemy.FindNewPath(_engine)
            }
            return
        }
        if(spawn.EnemyType=="Berserker"){
            var enemyModelPath = "assets/models/Berserker.glb"
            var enemyShape = ShapeFactory.MakeCapsuleShape(140.0, 50.0)

            if(spawn.SpawnLocationId > _spawnLocations.count) {
                System.error("Invalid spawn location ID %(spawn.SpawnLocationId)")
            }

            var spawnerEntity = this.FindLocation(spawn)
            var position = spawnerEntity.GetTransformComponent().translation

            for(i in 0...spawn.Count) {
                var enemy = _enemyList.add(BerserkerEnemy.new(_engine, position, Vec3.new(0.026, 0.026, 0.026), 4, enemyModelPath, enemyShape))
                enemy.FindNewPath(_engine)
            }
            return
        }
    }

    FindLocation(spawn) {
        var playerPosition = _player.GetTransformComponent().translation

        var closestDistance = 99999
        var furthestDistance = 0

        var spawnerEntity = _spawnLocations[0]
        if(spawn.SpawnLocationId < 0) {
            for(location in _spawnLocations) {
                var position = location.GetTransformComponent().translation
                var distance = Math.Distance(position, playerPosition)
                if(spawn.SpawnLocationId == SpawnLocationType.Closest && distance < closestDistance) {
                    closestDistance = distance
                    spawnerEntity = location
                }
                if(spawn.SpawnLocationId == SpawnLocationType.Furthest && distance > furthestDistance) {
                    furthestDistance = distance
                    spawnerEntity = location
                }
            }
        } else {
            spawnerEntity = _spawnLocations[spawn.SpawnLocationId]
        }
    
        return spawnerEntity
    }
}
