import "engine_api.wren" for Engine, ECS, Entity, Vec4, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/player.wren" for PlayerVariables


class PowerUpType {
    static NONE{0}
    static QUAD_DAMAGE {1}
    static DOUBLE_GUNS {2}    
}


class Station {
    construct new(engine, spawnPosition, stationManagerReference) {
        _stationManagerReference = stationManagerReference

        _interactRange = 10.0 // Range for the station to be interacted with by the player

        _rootEntity = engine.GetECS().NewEntity()
        _rootEntity.AddNameComponent().name = "Station"
        _rootEntity.AddEnemyTag()
        _rootEntity.AddAudioEmitterComponent()

        var transform = _rootEntity.AddTransformComponent()
        transform.translation = spawnPosition

        // var emitterFlags = SpawnEmitterFlagBits.eIsActive()
        // engine.GetParticles().SpawnEmitter(_rootEntity, EmitterPresetID.eSoulSheet(),emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.0, 0.0, 0.0))


        _time = 0.0 // Time since the station was spawned
        _ambientStationSound = "event:/SFX/StationAmbient"
        _ambientSoundEventInstance = null
        _powerUpType = PowerUpType.NONE


        _textOpacity = 0.0 // Transparency of the text
        _textColor = Vec3.new(1.0, 1.0, 1.0) // Color of the text
        _isActive = false
    }

    CheckRange(engine, playerPos, playerVariables, dt){
        var stationTransform = _rootEntity.GetTransformComponent()
        var stationPos = stationTransform.translation
        var distance = Math.Distance(stationPos, playerPos)
        if(_isActive == false){
            this.StopSound(engine) // Stop the sound if the station is not active
        }

        if(distance < _interactRange  && _isActive){

            if(_powerUpType == PowerUpType.QUAD_DAMAGE){
                engine.GetGame().GetHUD().SetPowerUpText("QUAD DAMAGE RELIC [COST: 5000]")
            }

            _textOpacity = _textOpacity + dt * 0.005
            _textOpacity = Math.Clamp(_textOpacity, 0.0, 1.0)
            _textColor = Math.MixVec3(_textColor, Vec3.new(1.0, 1.0, 1.0), 0.01)
            
            engine.GetGame().GetHUD().SetPowerUpTextColor(Vec4.new(_textColor.x,_textColor.y,_textColor.z,_textOpacity) )



            if(engine.GetInput().GetDigitalAction("Reload").IsPressed()){

                if(playerVariables.GetScore() >= 5000){

                    if(_powerUpType == PowerUpType.QUAD_DAMAGE){
                        _stationManagerReference.PlayQuadHumSound(engine) 
                    }

                    playerVariables.SetCurrentPowerUp(_powerUpType)
                    playerVariables.DecreaseScore(5000)
                    _time = 0.0
                    this.SetStatus(false)
                    this.SetPowerUpType(PowerUpType.NONE)

                    _stationManagerReference.anyActiveStation = false 

                    System.print("Picked up power up ")

                }else{
                    _textColor = Vec3.new(0.8, 0.0, 0.0) // Set the text color to red
                }


                
            }
            
        }else{
            _textOpacity = _textOpacity - dt * 0.005
            _textOpacity = Math.Clamp(_textOpacity, 0.0, 1.0)
            engine.GetGame().GetHUD().SetPowerUpTextColor(Vec4.new(1.0,1.0,1.0,_textOpacity) )

        }
    }

    PlaySound(engine, volume){
        if(_ambientSoundEventInstance == null){
            _ambientSoundEventInstance = engine.GetAudio().PlayEventLoop(_ambientStationSound)
            engine.GetAudio().SetEventVolume(_ambientSoundEventInstance, volume)
            var audioEmitter = _rootEntity.GetAudioEmitterComponent()
            audioEmitter.AddEvent(_ambientSoundEventInstance)
        }
    }

    StopSound(engine){
        if(_ambientSoundEventInstance != null){
            engine.GetAudio().StopEvent(_ambientSoundEventInstance)
            _ambientSoundEventInstance = null
        }
    }

    SetStatus(newStatus){
        _isActive = newStatus
    }

    GetStatus(){
        return _isActive
    }

    GetPowerUpType(){
        return _powerUpType
    }

    SetPowerUpType(newPowerUpType){
        _powerUpType = newPowerUpType
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
}

class StationManager {
    construct new(engine, player){
        _stationList = [] // List of souls
        _playerEntity = player // Reference to the player entity
        _maxLifeOfActiveStation = 30000.0 // Maximum lifetime of a station
        _intervalBetweenStations = 55000.0 // Time interval between station's spawn
        _anyActiveStation = false // Flag to check if any station is active

        // Load here the power up meshes and effects
        // Quad damage
        _quadDamageMeshEntity = engine.LoadModel("assets/models/quad_dmg.glb", false)
        _quadDamageTransparency = _quadDamageMeshEntity.AddTransparencyComponent()
        _quadDamageTransparency.transparency = 0.0 // Set the default transparency to 0.0
        _quadLightEntity = engine.GetECS().NewEntity()
        _quadLightEntity.AddNameComponent().name = "Quad Damage Light"
        var lightTransform = _quadLightEntity.AddTransformComponent()
        lightTransform.translation = Vec3.new(0.0, -0.32, 0.0)
        _pointLight = _quadLightEntity.AddPointLightComponent()
        _pointLight.intensity = 40
        _pointLight.range = 5.5
        _pointLight.color = Vec3.new(0.67, 0.06, 0.89)
        _quadDamageMeshEntity.AttachChild(_quadLightEntity)
        _quadHumEvent = "event:/SFX/QuadHum"

        // quad damage emitter
        _quadDamageEmitter = engine.GetECS().NewEntity()
        _quadDamageEmitter.AddNameComponent().name = "Quad Damage Emitter"
        var transform = _quadDamageEmitter.AddTransformComponent()
        transform.translation = Vec3.new(0.0, -200.0, 0.0)
        var emitterFlags = SpawnEmitterFlagBits.eIsActive()
        engine.GetParticles().SpawnEmitter(_quadDamageEmitter, EmitterPresetID.eQuadStation(),emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.0, 0.0, 0.0))
        
        //



        // Load the stations
        for(i in 0..3) {
            var entity = engine.GetECS().GetEntityByName("Station_%(i)")
            if(entity) {
                var position = entity.GetTransformComponent().translation
              
                this.AddStation(engine, position)
            }
        }

        _timer = _intervalBetweenStations
    }

    AddStation(engine, spawnPosition){
        _stationList.add(Station.new(engine, spawnPosition,this)) // Add the station to the list
    }

    anyActiveStation { _anyActiveStation }
    anyActiveStation=(value) { _anyActiveStation = value }

    PlayQuadHumSound(engine){
        var player = engine.GetECS().GetEntityByName("Camera")
        var audioEmitter = player.GetAudioEmitterComponent()
        var quadEventInstance = engine.GetAudio().PlayEventOnce(_quadHumEvent)
        engine.GetAudio().SetEventVolume(quadEventInstance, 2.4)
        audioEmitter.AddEvent(quadEventInstance)
    }

    Update(engine, playerVariables, dt){
        var playerTransform = _playerEntity.GetTransformComponent()
        var playerPos = playerTransform.translation
        playerPos.y = playerPos.y - 1.0

        var currentPowerUpColor =  engine.GetGame().GetHUD().GetPowerUpTextColor()

        //System.print("Current color:%(currentPowerUpColor.x),%(currentPowerUpColor.y),%(currentPowerUpColor.z),%(currentPowerUpColor.w)")
        var newOpacity = currentPowerUpColor.w - dt * 0.005
        newOpacity = Math.Clamp(newOpacity, 0.0, 1.0)

        engine.GetGame().GetHUD().SetPowerUpTextColor(Vec4.new(currentPowerUpColor.x,currentPowerUpColor.y,currentPowerUpColor.z,newOpacity) )
        // Timer to set a random station active
        _timer = _timer + dt
        if(_timer > _intervalBetweenStations){
            _timer = 0.0

            // Disable all stations first
            for(station in _stationList){
                station.time = 0.0
                station.SetStatus(false)
                station.SetPowerUpType(PowerUpType.NONE)
            }

            // Reset this flag
            _anyActiveStation = false
            

            // Enable a random one
            var randomIndex = Random.RandomIndex(0, 3)
            _stationList[randomIndex].SetStatus(true)

            // to be randomized when we add more power ups
            _stationList[randomIndex].SetPowerUpType(PowerUpType.QUAD_DAMAGE) // Set the power up type to quad damage
            _stationList[randomIndex].time = 0.0 // Reset the time for the station
            _quadDamageTransparency.transparency = 0.0 // Reset the transparency to 0.0

            var meshOffset = Vec3.new(0.0, 2.1, 0.0)
            _quadDamageMeshEntity.GetTransformComponent().translation =  _stationList[randomIndex].entity.GetTransformComponent().translation + meshOffset
            _quadDamageEmitter.GetTransformComponent().translation =  _stationList[randomIndex].entity.GetTransformComponent().translation + meshOffset + Vec3.new(0.0, 4.5, 0.0)
            System.print("Too much time has passed between stations, setting a new one active")
            //System.printAll(["New station is now available",randomIndex, _quadDamageMeshEntity.GetTransformComponent().translation.x, _quadDamageMeshEntity.GetTransformComponent().translation.y, _quadDamageMeshEntity.GetTransformComponent().translation.z]) //> 1[2, 3]4
        }


        // Update the stations
        for(station in _stationList){
            if(station.entity.IsValid()){
                station.CheckRange(engine, playerPos, playerVariables, dt) // Check if the station is within range of the player
                if(station.GetStatus()){
                    station.time = station.time + dt

                    station.PlaySound(engine, 1.6) // Play the sound if the station is active

                }

                // Deactivate the station if it has been around for too long 
                // This balance gameplay by not allowing the player to spam pickup powerups
                if(station.time > _maxLifeOfActiveStation ){
                    station.time = 0.0
                    station.SetStatus(false)
                    station.SetPowerUpType(PowerUpType.NONE)
                    _anyActiveStation = false
                    System.print("Station has been around for too long, deactivating it")

                }
                
                // Show visuals based on the power up type
                var powerUpType = station.GetPowerUpType()
                if(station.GetStatus()){

                    if(powerUpType == PowerUpType.QUAD_DAMAGE){
                        _quadDamageTransparency.transparency = Math.MixFloat(_quadDamageTransparency.transparency, 1.1, 0.005 )
                        _anyActiveStation = true 
                        // Set the other meshes transparency to 0.0 
                        // TO BE ADDED when the other power ups are added
                    }

                    break // No need to check other stations since we want at most one active
                }

            }
        }

        if(!_anyActiveStation){
            // hide all the pickups meshes and effects under the map
            _quadDamageMeshEntity.GetTransformComponent().translation = Vec3.new(0.0, -100.0, 0.0) // Move the mesh out of bounds
            _quadDamageEmitter.GetTransformComponent().translation = Vec3.new(0.0, -200.0, 0.0) // Move the emitter out of bounds

        }

        // Update quad damage mesh
        var bounce = Math.Sin(_timer * 0.003) * 0.0008 *dt
        var quadDamageTransform = _quadDamageMeshEntity.GetTransformComponent()
        quadDamageTransform.translation = Vec3.new(quadDamageTransform.translation.x,quadDamageTransform.translation.y + bounce,quadDamageTransform.translation.z)  // Make the soul bounce up and down
        quadDamageTransform.rotation = Math.ToQuat(Vec3.new(0.0, Math.Radians(_timer * 0.1),0.0 )) // Rotate the mesh

    }
}