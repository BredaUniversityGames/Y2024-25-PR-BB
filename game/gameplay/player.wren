import "gameplay/hud.wren" for WrenHUD, HitmarkerState
import "engine_api.wren" for Math, Stat, Stats, Achievements
import "gameplay/station.wren" for PowerUpType

class PlayerVariables {
    construct new(cppHud, engine) {
        _engine = engine
        _maxHealth = 100.0
        _health = _maxHealth
        _score = 0
        _ultDecayRate = 10
        _ultMaxCharge = 100
        _ultCharge = 0
        _ultActive = false
        _ultMaxChargeMultiplier = 4.0
        _wasUltReadyLastFrame = false
        _grenadeMaxCharge = 100
        _grenadeChargeRate = 20
        _grenadeCharge = 0
        _hud =  WrenHUD.new(cppHud)
        _hud.SetWaveCounter(0)

        _invincibilityMaxTime = 500
        _invincibilityTime = 0
        
        _cameraVariablesRef = null

        _hitmarkerState = HitmarkerState.normal
        _hitmarkTimer = 0
        
        _soulsIndicatorTimerCurrent = 0
        _soulsIndicatorTimerMax = 300
        _soulsIndicatorFadeIn = true
        
        _multiplier = 1.0
        _multiplierIncrement = 0.2

        _multiplierTimer = 0
        _multiplierMaxTime = 5000

        _consecutiveHits = 0
        _consecutiveMaxHits = 5    

        _godMode = false    

        _damageMultiplier = 1.0
        _currentPowerUp = PowerUpType.NONE  
        // Let's change this based on which PowerUp we have
        _gunSmokeRay = "Ray"
        _muzzleFlashRay = "Muzzle"
    }

    godMode=(v) { _godMode = v }
    godMode { _godMode }
    hud { _hud }
    hitmarkerState{_hitmarkerState}
    hitmarkTimer{_hitmarkTimer}
    health {_health}
    maxHealth {_maxHealth}
    score {_score}
    ultCharge {_ultCharge}
    ultMaxCharge {_ultMaxCharge}
    ultChargeRate {_ultChargeRate}
    ultDecayRate {_ultDecayRate}
    ultActive {_ultActive}
    grenadeCharge {_grenadeCharge}
    grenadeChargeRate {_grenadeChargeRate}
    grenadeMaxCharge {_grenadeMaxCharge}
    invincibilityTime {_invincibilityTime}
    invincibilityMaxTime {_invincibilityMaxTime}
    multiplier {_multiplier}
    multiplierIncrement {_multiplierIncrement}
    multiplierTimer {_multiplierTimer}
    multiplierMaxTime {_multiplierMaxTime}
    consecutiveHits {_consecutiveHits}
    consecutiveMaxHits {_consecutiveMaxHits}
    wasUltReadyLastFrame {_wasUltReadyLastFrame}
    
    cameraVariables {_cameraVariablesRef}
    cameraVariables=(value) {_cameraVariablesRef = value}

    hitmarkerState=(value) {_hitmarkerState = value}
    hitmarkTimer=(value){_hitmarkTimer = value}
    health=(value) {_health = value}
    score=(value) {_score = value}
    ultCharge=(value) {_ultCharge = value}
    ultActive=(value) {_ultActive = value}
    grenadeCharge=(value) {_grenadeCharge = value}
    invincibilityTime=(value) {_invincibilityTime = value}
    multiplier=(value) {_multiplier = value}
    multiplierIncrement=(value) {_multiplierIncrement = value}
    multiplierTimer=(value) {_multiplierTimer = value}
    multiplierMaxTime=(value) {_multiplierMaxTime = value}
    consecutiveHits=(value) {_consecutiveHits = value}
    consecutiveMaxHits=(value) {_consecutiveMaxHits = value}
    wasUltReadyLastFrame=(value) {_wasUltReadyLastFrame = value}

    IsInvincible() {
        return _invincibilityTime > 0 || _godMode
    }

    DecreaseHealth(value) {
        if (!this.IsInvincible()) {
            _health = Math.Max(_health - value, 0)
        }
    }

    IncreaseHealth(value) {
        _health = Math.Min(_health + value, _maxHealth)
    }

    IncreaseScore(value) {
        _score = _score + value

        var stat = _engine.GetSteam().GetStat(Stats.GOLD_CURRENCY_COLLECTED())
        if(stat != null) {
            stat.intValue = stat.intValue + value
        }
    }

    GetScore() {
        return _score
    }

    DecreaseScore(value) {
        _score = Math.Max(_score - value, 0)
    }

    SetCurrentPowerUp(powerUp) {
        _currentPowerUp = powerUp
    }

    GetCurrentPowerUp() {
        return _currentPowerUp
    }    

    SetGunSmokeRay(gunSmokeRay) {
        _gunSmokeRay = gunSmokeRay
    }
    GetGunSmokeRay() {
        return _gunSmokeRay
    }

    SetMuzzleFlashRay(muzzleFlashRay) {
        _muzzleFlashRay = muzzleFlashRay
    }

    GetMuzzleFlashRay() {
        return _muzzleFlashRay
    }

    SetDamageMultiplier(damageMultiplier) {
        _damageMultiplier = damageMultiplier
    }
    GetDamageMultiplier() {
        return _damageMultiplier
    }

    UpdateMultiplier() {
        _consecutiveHits = _consecutiveHits + 1
        _multiplierTimer = _multiplierMaxTime
        if (_consecutiveHits == 5) {
            _consecutiveHits = 0
            _multiplier = _multiplier + _multiplierIncrement
        }
    }
       
    UpdateUltCharge(multiplier) {
        _ultCharge =  Math.Min(_ultCharge + multiplier * Math.Min(2 * _multiplier, _ultMaxChargeMultiplier), _ultMaxCharge)
    }
}