import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

class Main {

    static Start(engine) {
        engine.GetAudio().LoadBank("assets/sounds/Master.bank")
        engine.GetAudio().LoadBank("assets/sounds/Master.strings.bank")
        engine.GetAudio().LoadBank("assets/sounds/SFX.bank")

        __wasGroundedLastFrame = false


        __counter = 0
        __frameTimer = 0
        __timer = 0
        __player = engine.GetECS().GetEntityByName("Camera")
        __playerController = engine.GetGame().CreatePlayerController(engine.GetPhysics(),engine.GetECS(),Vec3.new(0.0, 30.0, 0.0),0.85,0.5)
        __gun = engine.GetECS().GetEntityByName("AnimatedRifle")
        var gunAnimations = __gun.GetAnimationControlComponent()
        gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
        gunAnimations.Stop()

        if (__player) {
            System.print("Player is online!")

            var playerTransform = __player.GetTransformComponent()
            playerTransform.translation = Vec3.new(4.5, 35.0, 285.0)

            __player.AddAudioEmitterComponent()

            //var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity()
            //engine.GetParticles().SpawnEmitter(__player, EmitterPresetID.eTest(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(5.0, -1.0, -5.0))

            var gunTransform = __gun.GetTransformComponent()
            gunTransform.translation = Vec3.new(-0.4, -3.1, -1)
            gunTransform.rotation = Math.ToQuat(Vec3.new(0.0, -Math.PI(), 0.0))
        }

        __rayDistance = 1000.0
        __rayDistanceVector = Vec3.new(__rayDistance, __rayDistance, __rayDistance)
    }

    static Update(engine, dt) {
        __counter = __counter + 1

        var deltaTime = engine.GetTime().GetDeltatime()
        __frameTimer = __frameTimer + dt
        __timer = __timer + dt

        if (__frameTimer > 1000.0) {
            //System.print("%(__counter) Frames per second")
            __frameTimer = __frameTimer - 1000.0
            __counter = 0
        }

        if (engine.GetInput().GetDigitalAction("Shoot")) {
            var shootingInstance = engine.GetAudio().PlayEventOnce("event:/Weapons/Machine Gun")
            var audioEmitter = __player.GetAudioEmitterComponent()
            audioEmitter.AddEvent(shootingInstance)

            System.print("Playing is shooting")
        }

        if (engine.GetInput().GetDigitalAction("Shoot")) {
            // var playerTransform = __player.GetTransformComponent()
            // var direction = Math.ToVector(playerTransform.rotation)
            // var start = playerTransform.translation + direction * Vec3.new(2.0, 2.0, 2.0)
            // var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, __rayDistance)
            // var end = start + direction * __rayDistanceVector
            // if(!rayHitInfo.isEmpty) {
            //     end = rayHitInfo[0].position
            //     var entity = engine.GetECS().NewEntity()
            //     var transform = entity.AddTransformComponent()
            //     transform.translation = end
            //     var lifetime = entity.AddLifetimeComponent()
            //     lifetime.lifetime = 1000.0
            //     var emitterFlags = SpawnEmitterFlagBits.eIsActive()
            //     engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eTest(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
            // }
            

            // var length = (end - start).length()
            // var i = 5.0
            // while (i < length) {
            //     var entity = engine.GetECS().NewEntity()
            //     var transform = entity.AddTransformComponent()
            //     transform.translation = Math.Mix(start, end, i / length)
            //     var lifetime = entity.AddLifetimeComponent()
            //     lifetime.lifetime = 1000.0
            //     var emitterFlags = SpawnEmitterFlagBits.eIsActive()
            //     engine.GetParticles().SpawnEmitter(entity, EmitterPresetID.eTest(), emitterFlags, Vec3.new(0.0, 0.0, 0.0), Vec3.new(0.0, 0.0, 0.0))
            //     i = i + 5.0
            // }
        }


        var playerBody = __playerController.GetRigidbodyComponent()
        var velocity = engine.GetPhysics().GetVelocity(playerBody)

        var cameraRotation = __player.GetTransformComponent().rotation
        var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
        forward.y = 0.0

        var right = (cameraRotation.mul( Vec3.new(1.0, 0.0, 0.0))).normalize()
        
        // lets test for ground here
        var playerControllerPos = engine.GetPhysics().GetPosition(playerBody)
        var rayDirection = Vec3.new(0.0, -1.0, 0.0)
        var rayLength = 2.0

        var groundCheckRay = engine.GetPhysics().ShootRay(playerControllerPos, rayDirection, rayLength)

        var isGrounded = false
        for(hit in groundCheckRay) {

            var curva = hit.GetEntity(engine.GetECS()).GetEnttEntity()
            // System.print("Entity hit: %(curva)")
            // System.print("PlayerController: %(__playerController.GetEnttEntity())")
            if(hit.GetEntity(engine.GetECS()).GetEnttEntity() != __playerController.GetEnttEntity()) {
                isGrounded = true
                //System.print("PULA")

                break
            }
        }

      

        var movement = engine.GetInput().GetAnalogAction("Move")

        //this might not be translated correctly with controller
        var moveInputDir = Vec3.new(0.0,0.0,0.0)
        if(engine.GetInput().DebugGetKeyHeld(Keycode.eW())){
            moveInputDir = moveInputDir + forward
        }
        if(engine.GetInput().DebugGetKeyHeld(Keycode.eS())){
            moveInputDir = moveInputDir - forward
        }
        if(engine.GetInput().DebugGetKeyHeld(Keycode.eA())){
            moveInputDir = moveInputDir - right
        }
        if(engine.GetInput().DebugGetKeyHeld(Keycode.eD())){
            moveInputDir = moveInputDir + right
        }
        
        moveInputDir = moveInputDir.normalize()

        var isJumpHeld = engine.GetInput().DebugGetKeyHeld(Keycode.eSPACE())

        if(isGrounded && isJumpHeld) {
            velocity.y = 0.0
            velocity = velocity + Vec3.new(0.0, 8.20, 0.0)
            __wasGroundedLastFrame = false
        }else {
            __wasGroundedLastFrame = isGrounded
        }

        __wasGroundedLastFrame = isGrounded

        var maxSpeed = 9.0
        var sv_accelerate = 10.0
        var frameTime = engine.GetTime().GetDeltatime()

        var wishVel = moveInputDir * Vec3.new(maxSpeed,maxSpeed,maxSpeed)

        engine.GetPhysics().GravityFactor(playerBody,2.2)
        if(isGrounded){
            engine.GetPhysics().SetFriction(playerBody,12.0)
            var currentSpeed = Math.Dot(velocity, moveInputDir)
            
            var addSpeed = maxSpeed - currentSpeed
            if(addSpeed>0){
                var accelSpeed = sv_accelerate * frameTime * maxSpeed
                if(accelSpeed > addSpeed){
                    accelSpeed = addSpeed
                }

                velocity = velocity + Vec3.new(accelSpeed,accelSpeed,accelSpeed) * moveInputDir
            }
        }else{
            System.print("Player is in the air")
            var wishSpeed = wishVel.length()
            wishVel = wishVel.normalize()
            if(wishSpeed > 0.3){
                wishSpeed = 0.3
            }

            var currentSpeed = Math.Dot(velocity, wishVel)
            var addSpeed = wishSpeed - currentSpeed
            if(addSpeed > 0){
                var accelSpeed = maxSpeed*sv_accelerate*frameTime
                if(accelSpeed > addSpeed){
                    accelSpeed = addSpeed
                }
                velocity = velocity + wishVel * Vec3.new(accelSpeed,accelSpeed,accelSpeed)
            }
        }

        // Wall Collision Fix - Project velocity if collision occurs
        var wallCheckRays = engine.GetPhysics().ShootMultipleRays(playerControllerPos, velocity,1.25,3,35.0)

        for(hit in wallCheckRays) {
        if(hit.GetEntity(engine.GetECS()).GetEnttEntity() != __playerController.GetEnttEntity()) {
            var wallNormal = hit.normal

            //Clip velocity
            var backoff = Math.Dot(velocity, wallNormal) * 1.0
            velocity = velocity - wallNormal *  Vec3.new(backoff,backoff,backoff)
        }
        }

        engine.GetPhysics().SetVelocity(playerBody, velocity)

        var pos = engine.GetPhysics().GetPosition(playerBody)
        pos.y = pos.y + 0.5

        __player.GetTransformComponent().translation = pos






        if (engine.GetInput().GetDigitalAction("Jump")) {
            System.print("Player Jumped!")

        }

        var gunAnimations = __gun.GetAnimationControlComponent()
        if(engine.GetInput().GetDigitalAction("Reload") && gunAnimations.AnimationFinished()) {
            gunAnimations.Play("Armature|Armature|Reload", 1.0, false)
        }
        if(engine.GetInput().GetDigitalAction("Shoot")) {
            if(gunAnimations.AnimationFinished()) {
                gunAnimations.Play("Armature|Armature|Shoot", 2.0, false)
            }
        }


        if (movement.length() > 0) {
            System.print("Player is moving")
        }

        var key = Keycode.eA()
        if (engine.GetInput().DebugGetKey(key)) {
            System.print("[Debug] Player pressed A!")
        }
    }
}