import "engine_api.wren" for Engine, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input

class CameraVariables {

    construct new() {
        _shakeIntensity = 0.3
        _shakeOffset = Vec2.new(0.0, 0.0)
        _tiltFactor = 0   
        _slideFactorX = 0
        _recoilOffsetDesired = 0
        _recoilOffsetActual = 0
        _recoilMixFactor = 0.02
        _recoilDecayFactor = 0.02
    }

    shakeIntensity {_shakeIntensity}
    shakeOffset {_shakeOffset}
    tiltFactor {_tiltFactor}

    shakeIntensity=(value) {_shakeIntensity = value}
    shakeOffset=(value) {_shakeOffset = value}
    tiltFactor=(value) {_tiltFactor = value}
    
    AddRecoil(value){   
        _recoilOffsetDesired = _recoilOffsetDesired + value
    }

    Shake(engine, cameraEntity, timer) {
        if (_shakeIntensity > 0.001) {
            _shakeOffset = Vec2.new(Math.Sin(timer * 200.0) * 0.3, Math.Cos(timer * 200.0)).mulScalar(_shakeIntensity).mulScalar(0.3)

            _shakeIntensity = _shakeIntensity * 0.9

        } else {
            _shakeOffset = Vec2.new(0.0, 0.0)
        }

        cameraEntity.GetTransformComponent().translation = Vec3.new(_shakeOffset.x, _shakeOffset.y, 0.0)
    }
    
    ProcessRecoil(engine,cameraEntity,dt){
       
       //calculate recoil offset 
       _recoilOffsetActual = Math.MixFloat(_recoilOffsetActual,_recoilOffsetDesired,_recoilMixFactor*dt)
       
       _recoilOffsetActual = Math.Clamp(_recoilOffsetActual,0,45)
        cameraEntity.GetTransformComponent().rotation = Math.ToQuat(Vec3.new(Math.Radians(_recoilOffsetActual),0,0))
      _recoilOffsetDesired = Math.Clamp(_recoilOffsetDesired  -(_recoilDecayFactor*dt),0,45)
    }

    Tilt(engine, cameraEntity, dt) {
        var movement = engine.GetInput().GetAnalogAction("Move")
        var isSliding = engine.GetInput().GetDigitalAction("Slide").IsHeld()
        var tiltSpeed= 0.3
        
            
        if (Math.Abs(movement.x) < 0.0001) {
            _tiltFactor = Math.Clamp(_tiltFactor * 0.2,0.001,20.0)
        } else {
            _tiltFactor = Math.Clamp(_tiltFactor + (dt * 0.01) * -movement.x, -1, 1)   
        }

        if(isSliding) {
            tiltSpeed = 0.005
            if (Math.Abs(movement.x) < 0.0001) {
                _slideFactorX = Math.Clamp(_slideFactorX * 0.2, 2.5, 5)
            } else {
                _slideFactorX = Math.Clamp(_slideFactorX + (dt * 0.01) * -movement.x, -3, 3)   
            }

           
        }else{
            _slideFactorX = _slideFactorX * 0.2
           
        }
        
        var transform = cameraEntity.GetTransformComponent()
        var newRotation = Math.ToQuat(Vec3.new(0.0, 0.0, Math.Radians(_tiltFactor + _slideFactorX)))
        transform.rotation = Math.Slerp(transform.rotation, newRotation, Math.Clamp(dt * tiltSpeed, 0.0, 1.0))
    }

}