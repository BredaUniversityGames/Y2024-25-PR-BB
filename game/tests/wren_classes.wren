class Player {
    construct new(name, points) {
        _name = name
        _score = points
    }

    // Getter
    name { _name }

    // Setter
    name=(val) { 
        _name = val 
    }

    score { _score }
    score=(val) {
        _score = val 
    }
}