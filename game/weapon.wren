import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

class Weapons {
    static pistol {0}
    static shotgun {1}
    static knife {2}
}

class WeaponBase {
    construct new(engine) {
        _damage
        _range
        _attackSpeed
        _mesh
        _shootAnim
        _equipAnim
        _reloadAnim
        _shootSFX
        _equipSFX
        _reloadSFX
    }

    reload () {
        System.print("Base reload")
    }

    attack() {
        System.print("Base attack")
    }

    equip () {
        System.print("Base equip")    
    }

    // damage {_damage}
    // range {_range}
    // attackSpeed {_attackSpeed}
    // mesh {_mesh}
    // shootAnim {_shootAnim}
    // equipAnim {_equipAnim}
    // reloadAnim {_reloadAnim}
    // shootSFX {_shootSFX}
    // equipSFX {_equipSFX}
    // reloadSFX {_reloadSFX}


}

class Pistol is WeaponBase {
    construct new(engine) {
        _damage = 20
        _range = 50
        _attackSpeed = 0.2
        _maxAmmo = 6
        _ammo = _maxAmmo

        // animations

        // SFX

        // mesh
    }

    reload () {
        System.print("Pistol reload")
    }

    attack() {
        System.print("Pistol shoot")
    }

    equip () {
        System.print("Pistol equip")
    }
}


class Shotgun is WeaponBase {
    construct new(engine) {

    }
    
    reload () {
        System.print("Shotgun reload")
    }

    attack() {
        System.print("Shotgun shoot")
    }

    equip () {
        System.print("Shotgun equip")    
    }
}

class Knife is WeaponBase {
    construct new(engine) {

    }

    reload () {
        System.print("Knife reload")
    }

    attack() {
        System.print("Knife stab")
    }

    equip () {
        System.print("Knife equip")   
    }
}