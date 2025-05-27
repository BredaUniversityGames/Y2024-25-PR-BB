import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input, SpawnEmitterFlagBits, EmitterPresetID,ShapeFactory, Rigidbody, PhysicsObjectLayer, RigidbodyComponent, CollisionShape, Random
import "gameplay/player.wren" for PlayerVariables
import "gameplay/flash_system.wren" for FlashSystem


class Coin {
    construct new(engine, spawnPosition) {

        _minRange = 2.0 // Range for the coin to be picked up by the player
        _mediumRange = 3.0
        _maxRange = 4.0 // Range for the coin to start following the player

        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "Coin"
        _rootEntity.AddAudioEmitterComponent()

        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition
        transform.scale = Vec3.new(0.95, 0.95, 0.95)

        // Coin mesh
        _meshEntity = engine.LoadModel("assets/models/nug.glb", false)
        _rootEntity.AttachChild(_meshEntity)

        _lightEntity = engine.GetECS().NewEntity()
        _lightEntity.AddNameComponent().name = "Coin Light"
        var lightTransform = _lightEntity.AddTransformComponent()
        lightTransform.translation = Vec3.new(0.001, 0.22, 0.001)
        _pointLight = _lightEntity.AddPointLightComponent()
        _pointLight.intensity = 10
        _pointLight.range = 0.5
        _pointLight.color = Vec3.new(0.9, 0.9, 0.08)
        _rootEntity.AttachChild(_lightEntity)
        
        // Coin collision
              // Physics callback with the two wren entities as parameters
        var onEnterCoinSound = Fn.new { |self, other|

            if (other.GetRigidbodyComponent().GetLayer() == PhysicsObjectLayer.eSTATIC()) {
                if(_collisionSoundTimer > 400.0){
                    this.PlaySound(engine,0.45)
                    this.collisionSoundTimer = 0.0
                }
            }
            return
        }

        var colliderShape = ShapeFactory.MakeBoxShape(Vec3.new(0.45,0.45,0.45))
        var rb = Rigidbody.new(engine.GetPhysics(), colliderShape, PhysicsObjectLayer.eCOINS(), true)
        var body = _rootEntity.AddRigidbodyComponent(rb).OnCollisionEnter(onEnterCoinSound)

        body.SetGravityFactor(2.5)
        body.SetFriction(9.0)

        var newVelocity = Vec3.new(0.0, 0.0, 0.0)
        newVelocity.x = Random.RandomFloatRange(-7.0, 7.0)
        newVelocity.y =  Random.RandomFloatRange(8.0, 15.0)
        newVelocity.z = Random.RandomFloatRange(-7.0, 7.0)
        body.SetVelocity(newVelocity)

        // var newAngularVelocity = Vec3.new(0.0, 0.0, 0.0)
        // newAngularVelocity.x = Random.RandomFloatRange(-10.0, 10.0)
        // newAngularVelocity.y =  Random.RandomFloatRange(8.0, 15.0)
        // newAngularVelocity.z = Random.RandomFloatRange(-10.0, 10.0)
        // body.SetAngularVelocity(newAngularVelocity)

        body.SetAngularVelocity(Random.RandomVec3Range(-50.0, 50.0))

        _time = 0.0 // Time since the coin was spawned
        _collisionSoundTimer = 0.0 // Timer for the collision sound
        
        // New velocity-based arc system variables
        _hasStartedFollowing = false
        _velocity = Vec3.new(0.0,0.0,0.0)
        _gravity = Vec3.new(0, -0.098, 0) // gravity for arc
        _coinSpeed = 0.005

        _collectSoundEvent = "event:/SFX/Coin"
        this.PlaySound(engine,0.65)

    }

    CheckRange(engine, playerPos, playerVariables, flashSystem, dt){

        var coinTransform = _rootEntity.GetTransformComponent()
        var coinRigidbody = _rootEntity.GetRigidbodyComponent()

        var coinPos = coinTransform.translation
        var distance = Math.Distance(coinPos, playerPos)

        if(distance < _maxRange){
            _velocity = (playerPos - coinPos).normalize()

            var arcHeight = distance * 0.25
            _velocity = _velocity.mulScalar(5.0) + Vec3.new(0.0, arcHeight, 0.0)


            var progress = 1.0 - (distance / _maxRange)
            var easing = progress * progress // ease-out

            coinRigidbody.SetVelocity(_velocity.mulScalar(dt * _coinSpeed* easing)) // Move the coin towards the player

            if(distance <= _minRange){

                playerVariables.IncreaseScore(100) // Increase player health

                // Play audio
                var player = engine.GetECS().GetEntityByName("Camera")
                var eventInstance = engine.GetAudio().PlayEventOnce(_collectSoundEvent)
                engine.GetAudio().SetEventVolume(eventInstance, 1.0)
                var audioEmitter = player.GetAudioEmitterComponent()
                audioEmitter.AddEvent(eventInstance)

                // Play flash effect
                flashSystem.Flash(Vec3.new(0.89, 0.77, 0.06), 0.1)

                this.Destroy() // Destroy the coin after it is collected
               
            }
        }
    }

    PlaySound(engine, volume){
        var player = engine.GetECS().GetEntityByName("Camera")
        var eventInstance = engine.GetAudio().PlayEventOnce(_collectSoundEvent)
        engine.GetAudio().SetEventVolume(eventInstance, volume)
        var audioEmitter = _rootEntity.GetAudioEmitterComponent()
        audioEmitter.AddEvent(eventInstance)
    }

    Destroy(){
        
        var rb = _rootEntity.GetRigidbodyComponent()
        rb.SetTranslation(Vec3.new(0.0, -1000.0, 0.0)) // Move the coin out of the way
        
        // Add a lifetime component to the coin entity so it will get destroyed eventually
        
        var lifetime = _rootEntity.AddLifetimeComponent()
        lifetime.lifetime = 0.0
        var lifeTimeLight = _lightEntity.AddLifetimeComponent()
        lifeTimeLight.lifetime = 0.0

    }

    entity {
        return _rootEntity
    }

    time {_time}
    time=(value) { _time  = value}

    collisionSoundTimer {_collisionSoundTimer} 
    collisionSoundTimer=(value) { _collisionSoundTimer  = value}


}

class CoinManager {
    construct new(engine, player){
        _coinList = [] // List of coins
        _playerEntity = player // Reference to the player entity
        _maxLifeTimeOfCoin = 10000.0 // Maximum lifetime of a coin
    }

    SpawnCoin(engine, spawnPosition){
        _coinList.add(Coin.new(engine, spawnPosition)) // Add the coin to the list
    }

    Update(engine, playerVariables, flashSystem, dt){
        var playerTransform = _playerEntity.GetTransformComponent()
        var playerPos = playerTransform.translation
        playerPos.y = playerPos.y - 2.0

        for(coin in _coinList){
            if(coin.entity.IsValid()){
                coin.CheckRange(engine, playerPos, playerVariables,flashSystem, dt) // Check if the coin is within range of the player
                coin.time = coin.time + dt
                coin.collisionSoundTimer = coin.collisionSoundTimer + dt
                var body  = coin.entity.GetRigidbodyComponent()

                if(coin.time > _maxLifeTimeOfCoin && !coin.entity.GetLifetimeComponent()){
                    coin.Destroy() // Destroy the coin if it has been around for too long
                }
            } else {
                _coinList.removeAt(_coinList.indexOf(coin)) // Remove the coin from the list if it is no longer valid
            }
        }
    }
}