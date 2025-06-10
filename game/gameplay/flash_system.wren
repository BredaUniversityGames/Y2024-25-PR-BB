import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input
import "gameplay/player.wren" for PlayerVariables


class FlashSystem {
    construct new(engine) {
        _engine = engine
        _colorTarget = Vec3.new(0.0, 0.0, 0.0)
        _intensityTarget = 0.0

        _baseColor = Vec3.new(0.0, 0.0, 0.0)
        _baseIntensity = 0.0

        _flashTimer = 0.0
        
        _isGoingTowardsTarget = false
    }

    Update(engine, dt){
        var currentColor = engine.GetGame().GetFlashColor()
         var currentIntensity = engine.GetGame().GetFlashIntensity()

        if(_isGoingTowardsTarget){
            _flashTimer = _flashTimer + dt
            engine.GetGame().SetFlashColor(_colorTarget,Math.MixFloat(engine.GetGame().GetFlashIntensity(), _intensityTarget, 0.02))
        }else{
            _colorTarget = Math.MixVec3(currentColor, _baseColor, 0.02)
            _intensityTarget = _baseIntensity
            engine.GetGame().SetFlashColor(
                _colorTarget,
                Math.MixFloat(currentIntensity, _intensityTarget, 0.02)
            )
        }

        if(_flashTimer > 175){
            _isGoingTowardsTarget = false
            _flashTimer = 0.0
        }    
    }

    
    SetBaseColor(baseColor, baseIntensity) {
        // Called by powerup system every frame
        _baseColor = baseColor
        _baseIntensity = baseIntensity
        if (!_isGoingTowardsTarget) {
            _colorTarget = baseColor
            _intensityTarget = baseIntensity
        }
    }

    Flash(color, intensity){
        this.SetFlashColor(color,intensity)
        _isGoingTowardsTarget = true
    }

    SetFlashColor(color, intensity){
        _colorTarget = color
        _intensityTarget = intensity
    }
    
    GetIntensity(){
    return _colorTarget.y
    }

}