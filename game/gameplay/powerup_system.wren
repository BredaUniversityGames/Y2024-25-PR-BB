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
        

    }

    Update(engine, playerVariables, flashSystem, dt){
        // Update screen effect

        System.print(_powerUpTimer)
        var currentColor = engine.GetGame().GetFlashColor()
        flashSystem.SetBaseColor(_colorTarget, _intensityTarget)

        var playerPowerUp = playerVariables.GetCurrentPowerUp()
        if(playerPowerUp != PowerUpType.NONE){
            _powerUpTimer = _powerUpTimer - dt
            if(_powerUpTimer <= 0){
                playerVariables.SetCurrentPowerUp(PowerUpType.NONE)
                _powerUpTimer = _maxPowerUpTime
            }
        }


        if(playerPowerUp == PowerUpType.QUAD_DAMAGE){
            _intensityTarget = 0.5
            _colorTarget = _quadDamageColor
        }else { 
            _colorTarget = Vec3.new(0.0, 0.0, 0.0)
            _intensityTarget = 0.0
        }
    }
}
