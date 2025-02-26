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
    }

    health {_health}
    maxHealth {_maxHealth}
    score {_score}
    ultCharge {_ultCharge}
    ultMaxCharge {_ultMaxCharge}
    ultChargeRate {_ultChargeRate}
    ultDecayRate {_ultDecayRate}
    ultActive {_ultActive}

    health=(value) {_health = value}
    score=(value) {_score = value}
    ultCharge=(value) {_ultCharge = value}
    ultActive=(value) {_ultActive = value}
    
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