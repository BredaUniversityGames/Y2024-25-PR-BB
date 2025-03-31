import "engine_api.wren" for Math

class PlayerVariables {
    construct new() {
        _maxHealth = 100.0
        _health = _maxHealth
        _score = 0
        _ultChargeRate = 10
        _ultDecayRate = 10
        _ultMaxCharge = 100
        _ultCharge = 0
        _ultActive = false
        _grenadeMaxCharge = 100
        _grenadeChargeRate = 20
        _grenadeCharge = 0

        _invincibilityMaxTime = 500
        _invincibilityTime = 0
        
        _cameraVariablesRef = null
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
    
    cameraVariables {_cameraVariablesRef}
    cameraVariables=(value) {_cameraVariablesRef = value}

    health=(value) {_health = value}
    score=(value) {_score = value}
    ultCharge=(value) {_ultCharge = value}
    ultActive=(value) {_ultActive = value}
    grenadeCharge=(value) {_grenadeCharge = value}
    invincibilityTime=(value) {_invincibilityTime = value}

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
}