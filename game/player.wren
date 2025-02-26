import "engine_api.wren" for Math

class PlayerVariables {
    construct new() {
        _maxHealth = 100.0
        _health = _maxHealth
        _score = 0
    }

    health {_health}
    score {_score}

    health=(value) {_health = value}
    score=(value) {_score = value}

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