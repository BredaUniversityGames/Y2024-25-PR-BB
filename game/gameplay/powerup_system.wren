import "engine_api.wren" for Engine, ECS, Entity, Vec4, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input, SpawnEmitterFlagBits, EmitterPresetID, Random
import "gameplay/player.wren" for PlayerVariables
import "gameplay/station.wren" for PowerUpType, Station, StationManager
import "gameplay/flash_system.wren" for FlashSystem

class PowerUpSystem {
    construct new() {
        _quadDamageColor = Vec3.new(0.67, 0.06, 0.89)

        _intensityTarget = 0.0
        _colorTarget = Vec3.new(0.0, 0.0, 0.0)

        _maxPowerUpTime = 15000
        _powerUpTimer = _maxPowerUpTime

        _timerTextOpacity = 0.0
        _timerTextColor = Vec3.new(1.0, 1.0, 1.0)


    }

    Update(engine, playerVariables, flashSystem, dt){
        // Update screen effect
        var currentColor = engine.GetGame().GetFlashColor()
        flashSystem.SetBaseColor(_colorTarget, _intensityTarget)

        // Update UI text for timer
        engine.GetGame().GetHUD().SetPowerUpTimerTextColor(Vec4.new(_timerTextColor.x,_timerTextColor.y,_timerTextColor.z,_timerTextOpacity) )
        
        var timerInSeconds = Math.Round(_powerUpTimer / 1000)
        if(timerInSeconds <= 0){
           timerInSeconds = 0 
        } 
        engine.GetGame().GetHUD().SetPowerUpTimerText("%(timerInSeconds)")
        


        var playerPowerUp = playerVariables.GetCurrentPowerUp()
        if(playerPowerUp != PowerUpType.NONE){
            _powerUpTimer = _powerUpTimer - dt
            //make it go towards red when it reaches 0
            var t = 1.0 - Math.Clamp(_powerUpTimer / _maxPowerUpTime, 0.0, 1.0)
            _timerTextColor = Math.MixVec3(_timerTextColor, Vec3.new(1.0, 0.0, 0.0), t/1000.0)


            if(_powerUpTimer <= 0){
                playerVariables.SetCurrentPowerUp(PowerUpType.NONE)
            }

            
            _timerTextOpacity = _timerTextOpacity + dt * 0.005
            _timerTextOpacity = Math.Clamp(_timerTextOpacity, 0.0, 1.0)


        }else{
            _colorTarget = Vec3.new(0.0, 0.0, 0.0)
            _intensityTarget = 0.0

            if(_timerTextOpacity <= 0.01){
                _powerUpTimer = _maxPowerUpTime
                _timerTextColor = Vec3.new(1.0, 1.0, 1.0)
            }

            _timerTextOpacity = _timerTextOpacity - dt * 0.005
            _timerTextOpacity = Math.Clamp(_timerTextOpacity, 0.0, 1.0)

            //default the rest of the effects to normal
            playerVariables.SetGunSmokeRay(EmitterPresetID.eRay())

            // reset stats
            playerVariables.SetDamageMultiplier(1.0)
        }


        if(playerPowerUp == PowerUpType.QUAD_DAMAGE){
            _intensityTarget = 0.5
            _colorTarget = _quadDamageColor

            playerVariables.SetDamageMultiplier(4.0)
            playerVariables.SetGunSmokeRay(EmitterPresetID.eRayQuadDamage())
        }
    }
}
