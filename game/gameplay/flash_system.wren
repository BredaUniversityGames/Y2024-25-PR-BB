import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Math, AnimationControlComponent, TransformComponent, Input, SpawnEmitterFlagBits, EmitterPresetID
import "gameplay/player.wren" for PlayerVariables


class FlashSystem {
    construct new(engine) {
        _engine = engine
        _colorTarget = Vec3.new(0.0, 0.0, 0.0)
        _intensityTarget = 0.0

        _flashTimer = 0.0

        _isGoingTowardsTarget = false
    }

    Update(engine, dt){
        var currentColor = engine.GetGame().GetFlashColor()

        if(_isGoingTowardsTarget){
            _flashTimer = _flashTimer + dt
        }

        engine.GetGame().SetFlashColor(Math.MixVec3(currentColor, _colorTarget, 0.02),Math.MixFloat(engine.GetGame().GetFlashIntensity(), _intensityTarget, 0.02))
        if(!_isGoingTowardsTarget){
            this.SetFlashColor(_colorTarget, 0.0)
        }

        if(_flashTimer > 200){
            _isGoingTowardsTarget = false
            _flashTimer = 0.0
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

}