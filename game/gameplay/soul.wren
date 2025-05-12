import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input, SpawnEmitterFlagBits, EmitterPresetID
import "gameplay/player.wren" for PlayerVariables


class Soul {
    construct new(engine, spawnPosition) {

        _minRange = 1.0 // Range for the soul to be picked up by the player
        _mediumRange = 7.0
        _maxRange = 10.0 // Range for the soul to start following the player

        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "Soul"
        _rootEntity.AddEnemyTag()
        _rootEntity.AddAudioEmitterComponent()

        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition

        var emitterFlags = SpawnEmitterFlagBits.eIsActive()
        engine.GetParticles().SpawnEmitter(_rootEntity, EmitterPresetID.eSoulSheet(),emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.0, 0.0, 0.0))

        
        // New velocity-based arc system variables
        _hasStartedFollowing = false
        _velocity = Vec3.new(0.0,0.0,0.0)
        _gravity = Vec3.new(0, -0.098, 0) // gravity for arc
        _soulSpeed = 0.005
    }

    CheckRange(engine, playerPos, playerVariables, dt){
        var soulTransform = _rootEntity.GetTransformComponent()
        var soulPos = soulTransform.translation
        var distance = Math.Distance(soulPos, playerPos)


        if(distance < _maxRange){
            _velocity = (playerPos - soulPos).normalize()

            var arcHeight = distance * 0.25
            _velocity = _velocity.mulScalar(5.0) + Vec3.new(0.0, arcHeight, 0.0)


            var progress = 1.0 - (distance / _maxRange)
            var easing = progress * progress // ease-out

            soulTransform.translation = soulTransform.translation + _velocity.mulScalar(dt * _soulSpeed* easing)// Move the soul towards the player

            if(distance < _minRange){
                if(playerVariables.health < playerVariables.maxHealth){
                    playerVariables.IncreaseHealth(1) // Increase player health
                }
                soulTransform.translation = Vec3.new(0.0, -100.0, 0.0) // Move the soul out of the way
                // Add a lifetime component to the soul entity so it will get destroyed eventually
                var lifetime = _rootEntity.AddLifetimeComponent()
                lifetime.lifetime = 50.0
            }
        }
    }

    entity {
        return _rootEntity
    }
}

class SoulManager {
    construct new(engine, player){
        _soulList = [] // List of souls
        _playerEntity = player // Reference to the player entity
    }

    SpawnSoul(engine, spawnPosition){
        _soulList.add(Soul.new(engine, spawnPosition)) // Add the soul to the list
    }

    Update(engine, playerVariables, dt){
        var playerTransform = _playerEntity.GetTransformComponent()
        var playerPos = playerTransform.translation
        playerPos.y = playerPos.y - 1.0

        for(soul in _soulList){
            if(soul.entity.IsValid()){
                soul.CheckRange(engine, playerPos, playerVariables, dt) // Check if the soul is within range of the player
            } else {
                _soulList.removeAt(_soulList.indexOf(soul)) // Remove the soul from the list if it is no longer valid
            }
        }
    }
}