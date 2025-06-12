import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input, SpawnEmitterFlagBits, PhysicsObjectLayer
import "gameplay/player.wren" for PlayerVariables
import "gameplay/flash_system.wren" for FlashSystem

class SoulType {
    static SMALL{0}
    static BIG {1} 
}
class Soul {
    construct new(engine, spawnPosition, type) {
        _soulType = type

        _minRange = 1.0 // Range for the soul to be picked up by the player
        _mediumRange = 7.0
        _maxRange = 10.0 // Range for the soul to start following the player

        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "Soul"
        _rootEntity.AddAudioEmitterComponent()
        _rootEntity.AddTransparencyComponent()

        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition
        _targetPosition = spawnPosition

        var rayHitInfo = engine.GetPhysics().ShootRay(spawnPosition, Vec3.new(0, -1.0, 0), 100)
        if(!rayHitInfo.isEmpty) {
            for (rayHit in rayHitInfo) {
                var hitEntity = rayHit.GetEntity(engine.GetECS())
                if(hitEntity.GetRigidbodyComponent().GetLayer() == PhysicsObjectLayer.eSTATIC()) {
                    _targetPosition = rayHit.position + Vec3.new(0, 1.0, 0)
                    break
                }
            }
        }

        _lightEntity = engine.GetECS().NewEntity()
        _lightEntity.AddNameComponent().name = "Soul Light"
        var lightTransform = _lightEntity.AddTransformComponent()
        _pointLight = _lightEntity.AddPointLightComponent()

        _rootEntity.AttachChild(_lightEntity)

        var emitterFlags = SpawnEmitterFlagBits.eIsActive()

        if(_soulType == SoulType.SMALL){
            _pointLight.intensity = 10
            _pointLight.range = 3.0
            _pointLight.color = Vec3.new(0.23, 0.71, 0.36)
            engine.GetParticles().SpawnEmitter(_rootEntity, "SoulSheet",emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.0, 0.0, 0.0))
        }

        if(_soulType == SoulType.BIG){
            _pointLight.intensity = 15
            _pointLight.range = 5.0
            _pointLight.color = Vec3.new(1,1,1)
            engine.GetParticles().SpawnEmitter(_rootEntity, "SoulSheetBig",emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.0, 0.0, 0.0))
        }


        _time = 0.0 // Time since the soul was spawned

        // New velocity-based arc system variables
        _hasStartedFollowing = false
        _velocity = Vec3.new(0.0,0.0,0.0)
        _gravity = Vec3.new(0, -0.098, 0) // gravity for arc
        _soulSpeed = 0.005
        _maxLightIntensity = _pointLight.intensity

        _collectSoundEvent = "event:/SFX/Soul"
    }

    SetTransparency(value){
            _rootEntity.GetTransparencyComponent().transparency = value // Set the transparency of the soul particles
        }

    SetLightIntensity(value){
            _pointLight.intensity = value // Set the intensity of the soul light
        }

    CheckRange(engine, playerPos, playerVariables,flashSystem, dt){
        var soulTransform = _rootEntity.GetTransformComponent()
        var soulPos = soulTransform.translation
        var distance = Math.Distance(soulPos, playerPos)


        if(distance < _maxRange){
            _velocity = (playerPos - soulPos).normalize()

            var rayHitInfo = engine.GetPhysics().ShootRay(soulTransform.GetWorldTranslation(), Vec3.new(0, -1.0, 0), 100)
            if(!rayHitInfo.isEmpty) {
                for (rayHit in rayHitInfo) {
                    var hitEntity = rayHit.GetEntity(engine.GetECS())
                    if(hitEntity.GetRigidbodyComponent().GetLayer() == PhysicsObjectLayer.eSTATIC()) {
                        _targetPosition = rayHit.position + Vec3.new(0, 1.0, 0)
                        break
                    }
                }
            }

            var arcHeight = distance * 0.25
            _velocity = _velocity.mulScalar(5.0) + Vec3.new(0.0, arcHeight, 0.0)


            var progress = 1.0 - (distance / _maxRange)
            var easing = progress * progress // ease-out

            soulTransform.translation = soulTransform.translation + _velocity.mulScalar(dt * _soulSpeed* easing)// Move the soul towards the player

            if(distance < _minRange){
                if(playerVariables.health < playerVariables.maxHealth){

                    if(_soulType == SoulType.SMALL){
                        playerVariables.IncreaseHealth(2) // Increase player health
                    }

                    if(_soulType == SoulType.BIG){
                        playerVariables.IncreaseHealth(6) // Increase player health
                    }
                }

                 // Play audio
                var player = engine.GetECS().GetEntityByName("Camera")
                var eventInstance = engine.GetAudio().PlayEventOnce(_collectSoundEvent)
                engine.GetAudio().SetEventVolume(eventInstance, 5.0)
                var audioEmitter = player.GetAudioEmitterComponent()
                audioEmitter.AddEvent(eventInstance)

                // Play flash effect

                playerVariables.hud.TriggerSoulIndicatorAnimation()

                if(_soulType == SoulType.SMALL){
                    flashSystem.Flash(Vec3.new(0.23, 0.71, 0.36),0.35)
                }

                if(_soulType == SoulType.BIG){
                    flashSystem.Flash(Vec3.new(0.23, 0.71, 0.36),0.75)
                }
               

                this.Destroy() // Destroy the soul after it is collected

            }
        }else {
            var fall = Math.MixFloat(soulTransform.translation.y, _targetPosition.y, dt * 0.001)
            var bounce = Math.Sin(_time * 0.003) * 0.0008 *dt
            soulTransform.translation = Vec3.new(soulTransform.translation.x,fall + bounce,soulTransform.translation.z)  // Make the soul bounce up and down
        }
    }

    Destroy(){
        var soulTransform = _rootEntity.GetTransformComponent()
        soulTransform.translation = Vec3.new(0.0, -100.0, 0.0) // Move the soul out of the way
        // Add a lifetime component to the soul entity so it will get destroyed eventually
        var lifetime = _rootEntity.AddLifetimeComponent()
        lifetime.lifetime = 50.0
    }

    entity {
        return _rootEntity
    }

    time {_time}
    time=(value) { _time  = value}
    maxLightIntensity {_maxLightIntensity}
}

class SoulManager {
    construct new(engine, player){
        _soulList = [] // List of souls
        _playerEntity = player // Reference to the player entity
        _maxLifeTimeOfSoul = 20000.0 // Maximum lifetime of a soul
        _maxAppearTimeSoul = 1000.0
        _maxDisappearTimeSoul = 2000.0
    }

    SpawnSoul(engine, spawnPosition,type){
        _soulList.add(Soul.new(engine, spawnPosition,type)) // Add the soul to the list
    }

    Update(engine, playerVariables,flashSystem, dt){
        var playerTransform = _playerEntity.GetTransformComponent()
        var playerPos = playerTransform.translation
        playerPos.y = playerPos.y - 1.0

        for(soul in _soulList){
            if(soul.entity.IsValid()){
                soul.CheckRange(engine, playerPos, playerVariables, flashSystem, dt) // Check if the soul is within range of the player
                soul.time = soul.time + dt

                soul.SetTransparency(1.0)

                // fade in the souls
                if(soul.time < _maxAppearTimeSoul) {
                    soul.SetTransparency(soul.time / _maxAppearTimeSoul)
                }

                // fade out the souls
                var fadeStart = _maxLifeTimeOfSoul * (2.0 / 3.0)
                var fadeDuration = _maxLifeTimeOfSoul * (1.0 / 3.0)
                if(soul.time > fadeStart){
                    var t = (soul.time - fadeStart) / fadeDuration
                    t = Math.Min(Math.Max(t, 0), 1) // Clamp between 0 and 1
                    var transparency = 1.0 - t
                    var lightIntensity = (1.0 - t) * soul.maxLightIntensity
                    soul.SetTransparency(transparency) // Set the transparency of the soul particles
                    soul.SetLightIntensity(lightIntensity) // Set the intensity of the soul light
                }

                if(soul.time > _maxLifeTimeOfSoul && !soul.entity.GetLifetimeComponent()){
                    soul.Destroy() // Destroy the soul if it has been around for too long
                }
            } else {
                _soulList.removeAt(_soulList.indexOf(soul)) // Remove the soul from the list if it is no longer valid
            }
        }
    }
}
