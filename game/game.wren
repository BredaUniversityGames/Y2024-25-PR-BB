import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, MathUtil, AnimationControlComponent, Animation, TransformComponent, Input, Keycode

class Main {

    static Start(engine) {
        __counter = 0
        __frameTimer = 0
        __timer = 0
        __player = engine.GetECS().GetEntityByName("Camera")
        __gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        __firstShot = true

        if (__player) {
            System.print("Player is online!")

            var playerTransform = __player.GetTransformComponent()
            playerTransform.translation = Vec3.new(4.5, 35.0, 285.0)

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = MathUtil.ToQuat(Vec3.new(0.0, -MathUtil.PI(), 0.0))
        }
    }

    static Update(engine, dt) {
        __counter = __counter + 1

        var deltaTime = engine.GetTime().GetDeltatime()
        __frameTimer = __frameTimer + dt
        __timer = __timer + dt

        if (__frameTimer > 1000.0) {
            System.print("%(__counter) Frames per second")
            __frameTimer = __frameTimer - 1000.0
            __counter = 0
        }
        
        if (engine.GetInput().GetDigitalAction("Jump")) {
            System.print("Player Jumped!")

        }

        var gunAnimations = __gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload")) {
            gunAnimations.activeAnimation = 3
        }
        var shootAnim = gunAnimations.GetAnimation(4)
        if(engine.GetInput().GetDigitalAction("Shoot")) {
            if(gunAnimations.activeAnimation != 4) {
                gunAnimations.activeAnimation = 4
            }
            shootAnim.looping = true

            if(__firstShot) {
                shootAnim.time = 0.0
            }

            __firstShot = false
        } else {
            __firstShot = true
            if(gunAnimations.activeAnimation == 4) {
                var shootAnim = gunAnimations.GetAnimation(4)
                shootAnim.looping = false
            }
        }

        var movement = engine.GetInput().GetAnalogAction("Move")

        if (movement.length() > 0) {
            System.print("Player is moving")
        }

        var key = Keycode.eA()
        if (engine.GetInput().DebugGetKey(key)) {
            System.print("[Debug] Player pressed A!")
        }
    }
}