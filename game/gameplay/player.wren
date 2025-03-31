import "engine_api.wren" for Math

class PlayerVariables {
    construct new() {
        _maxHealth = 100.0
        _health = _maxHealth
        _score = 0
        _ultDecayRate = 10
        _ultMaxCharge = 100
        _ultCharge = 0
        _ultActive = false
        _ultMaxChargeMultiplier = 4.0
        _grenadeMaxCharge = 100
        _grenadeChargeRate = 20
        _grenadeCharge = 0

        _invincibilityMaxTime = 500
        _invincibilityTime = 0
        
        _cameraVariablesRef = null

        _multiplier = 1.0
        _multiplierIncrement = 0.2

        _multiplierTimer = 0
        _multiplierMaxTime = 5000

        _consecutiveHits = 0
        _consecutiveMaxHits = 5        
    }

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
    
    cameraVariables {_cameraVariablesRef}
    cameraVariables=(value) {_cameraVariablesRef = value}

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

    IsInvincible() {
        return _invincibilityTime > 0
    }

    DecreaseHealth(value) {
        _health = Math.Max(_health - value, 0)
    }

    IncreaseHealth(value) {
        _health = Math.Min(_health + value, _maxHealth)
    }

    IncreaseScore(value) {
        _score = _score + value
    }    

    UpdateMultiplier() {
        _consecutiveHits = _consecutiveHits + 1
        _multiplierTimer = _multiplierMaxTime
        if (_consecutiveHits == 5) {
            _consecutiveHits = 0
            _multiplier = _multiplier + _multiplierIncrement
        }
    }

    UpdateUltCharge() {
        _ultCharge = Math.Min(_ultCharge + Math.Min(2 * _multiplier, _ultMaxChargeMultiplier), _ultMaxCharge)
    }
}