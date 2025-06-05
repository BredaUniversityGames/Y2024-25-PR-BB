import "engine_api.wren" for Engine, Entity, Vec3, Vec2,  Math
import "gameplay/player.wren" for PlayerVariables

class HitmarkerState {
    static normal {0}
    static crit {1}    
}

class PingPongTween {
    construct new(maxTime, minVal, maxVal){
        _timerCurrent = 0
        _timerMax = maxTime
        _doReverse = true
        _minVal = minVal
        _maxVal = maxVal       
    }
    
    TriggerFromStart(){
        _timerCurrent = 0
        _doReverse = false
    }
    
    Update(dt){
        if(!_doReverse){
            _timerCurrent = Math.Clamp(_timerCurrent + dt, 0,_timerMax)
            _doReverse = _timerCurrent == _timerMax             
        }else{
            _timerCurrent = Math.Clamp(_timerCurrent - dt, 0,_timerMax)
        } 
    }
    
    GetValue(){
        var timeFract =  _timerCurrent / _timerMax
        var valFract = (_maxVal-_minVal) * timeFract
        return _minVal + valFract
    }
}

class LinearTween {
    construct new(maxTime,minVal,maxVal){
        _timerCurrent = maxTime
        _timerMax = maxTime
        _minVal = minVal
        _maxVal = maxVal     
    }
    
    TriggerFromStart(){
        _timerCurrent = 0
    }
    
    Update(dt){
        _timerCurrent = Math.Clamp(_timerCurrent + dt, 0,_timerMax)  
    }
        
    GetValue(){
        var timeFract =  _timerCurrent / _timerMax
        var valFract = (_maxVal-_minVal) * timeFract
        return _minVal + valFract
    }
}

class WrenHUD {
    
    construct new(hud) {
        _waveFlashTween = PingPongTween.new(200,0,1)
        _dashColorRefillTween = PingPongTween.new(75,1,5)
        _soulIndicatorOpacityTween = PingPongTween.new(100,0,1)

        _hud = hud
    }
    
    Update(engine, dt, playerMovement, playerVariables, currentAmmo, maxAmmo){
        _waveFlashTween.Update(dt)
        _dashColorRefillTween.Update(dt)
        _soulIndicatorOpacityTween.Update(dt)

        // Wave transition
        _hud.PlayWaveCounterIncrementAnim(_waveFlashTween.GetValue())

        // Dash charges
        _hud.UpdateDashCharges(playerMovement.currentDashCount)
        var timeTillNextCharge = 1-(playerMovement.currentDashRefillTime / 3000)
        var colorintenity = _dashColorRefillTween.GetValue()
        var color = Vec3.new(colorintenity,colorintenity,colorintenity)
        var opacity = Math.Clamp(timeTillNextCharge,0,1)
        _hud.SetDashChargeColor(playerMovement.currentDashCount,Vec3.new(1,1,1),opacity)
        _hud.SetDashChargeColor(playerMovement.currentDashCount-1,color,1)

        _hud.UpdateHealthBar(playerVariables.health / playerVariables.maxHealth)
        _hud.UpdateAmmoText(currentAmmo,maxAmmo)
        _hud.UpdateScoreText(playerVariables.score)
        _hud.ShowHitmarker(playerVariables.hitmarkTimer > 0 && playerVariables.hitmarkerState == HitmarkerState.normal)
        _hud.ShowHitmarkerCrit(playerVariables.hitmarkTimer > 0 && playerVariables.hitmarkerState == HitmarkerState.crit)
    

        _hud.SetSoulsIndicatorOpacity(_soulIndicatorOpacityTween.GetValue())
    }
    
    IncrementWaveCounter(CurrentWave){
        _hud.SetWaveCounterText(CurrentWave+1)
        _waveFlashTween.TriggerFromStart()
    }
    
    TriggerDashChargeRefillAnimation(){
       _dashColorRefillTween.TriggerFromStart()
    }
    
    TriggerSoulIndicatorAnimation(){
        _soulIndicatorOpacityTween.TriggerFromStart()
    }
}
