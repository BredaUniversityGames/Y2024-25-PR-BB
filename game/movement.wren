import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

class MovementClass{


    construct new(newHasDashed, newDashTimer){
        System.print("Movement Class Constructed")
        hasDashed = newHasDashed
        dashTimer = newDashTimer
    }


//getters
    hasDashed {_hasDashed}
    dashTimer {_dashTimer}
//setters
    hasDashed=(value) { _hasDashed  = value}
    dashTimer=(value) { _dashTimer = value}


    Movement(engine, playerController, player, wasGroundedLastFrame){

        var cheats = playerController.GetCheatsComponent()
        if(cheats.noClip == true){
            return
        }

        var playerBody = playerController.GetRigidbodyComponent()
        var velocity = engine.GetPhysics().GetVelocity(playerBody)

        var cameraRotation = player.GetTransformComponent().rotation
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


            if(hit.GetEntity(engine.GetECS()).GetEnttEntity() != playerController.GetEnttEntity()) {
                isGrounded = true
                break
            }
        }



        var movement = engine.GetInput().GetAnalogAction("Move")

        var moveInputDir = Vec3.new(0.0,0.0,0.0)
        moveInputDir = forward * Vec3.new(movement.y,movement.y,movement.y) + right * Vec3.new(movement.x,movement.x,movement.x)
        moveInputDir = moveInputDir.normalize()

        if(movement.length() > 0.1){
            engine.GetPhysics().SetFriction(playerBody, 0.0)
        }else{
            engine.GetPhysics().SetFriction(playerBody, 12.0)
        }

        var isJumpHeld = engine.GetInput().GetDigitalAction("Jump").IsHeld()


        if(isGrounded && isJumpHeld) {
            velocity.y = 0.0
            velocity = velocity + Vec3.new(0.0, 8.20, 0.0)
            wasGroundedLastFrame = false
        }else {
            wasGroundedLastFrame = isGrounded
        }
        wasGroundedLastFrame = isGrounded

        var maxSpeed = 11.0
        var sv_accelerate = 8.5
        var frameTime = engine.GetTime().GetDeltatime()
        var wishVel = moveInputDir * Vec3.new(maxSpeed,maxSpeed,maxSpeed)

        engine.GetPhysics().GravityFactor(playerBody,2.2)
        if(isGrounded && hasDashed == false){

            var currentSpeed = Math.Dot(velocity, moveInputDir)

            var addSpeed = maxSpeed - currentSpeed
            if (addSpeed > 0) {
                var accelSpeed = sv_accelerate * frameTime * maxSpeed
                if (accelSpeed > addSpeed) {
                    accelSpeed = addSpeed
                }
                //velocity = velocity + moveInputDir * Vec3.new(accelSpeed,accelSpeed,accelSpeed)
                velocity = velocity + Vec3.new(accelSpeed,accelSpeed,accelSpeed) * moveInputDir
            }

            var speed = velocity.length()
            if (speed > maxSpeed) {
                var factor = maxSpeed / speed
                velocity.x = velocity.x * factor

                velocity.z = velocity.z * factor
            }
                
        }else{
            
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
        if(hit.GetEntity(engine.GetECS()).GetEnttEntity() != playerController.GetEnttEntity()) {
            var wallNormal = hit.normal

            //Clip velocity
            var backoff = Math.Dot(velocity, wallNormal) * 1.0
            velocity = velocity - wallNormal *  Vec3.new(backoff,backoff,backoff)
        }
        }

        engine.GetPhysics().SetVelocity(playerBody, velocity)
        var pos = engine.GetPhysics().GetPosition(playerBody)
        pos.y = pos.y + 0.5
        player.GetTransformComponent().translation = pos
    }

    Dash(engine, dt, playerController, player){
        var playerBody = playerController.GetRigidbodyComponent()
        var velocity = engine.GetPhysics().GetVelocity(playerBody)
        var dashForce = 14.0
        if(engine.GetInput().GetDigitalAction("Dash").IsPressed()){

            hasDashed = true
            var cameraRotation = player.GetTransformComponent().rotation
            var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
            forward.y = 0.0
            var right = (cameraRotation.mul( Vec3.new(1.0, 0.0, 0.0))).normalize()
            var movement = engine.GetInput().GetAnalogAction("Move")

            var moveInputDir = Vec3.new(0.0,0.0,0.0)
            moveInputDir = forward * Vec3.new(movement.y,movement.y,movement.y) + right * Vec3.new(movement.x,movement.x,movement.x)
            moveInputDir = moveInputDir.normalize()

            if(moveInputDir.length() > 0.01){
                velocity = velocity + (moveInputDir * Vec3.new(dashForce,dashForce,dashForce))
                engine.GetPhysics().SetVelocity(playerBody, velocity)
            }else{
                velocity = velocity + (forward * Vec3.new(dashForce,dashForce,dashForce))
                engine.GetPhysics().SetVelocity(playerBody, velocity)
            }
        }

        if(hasDashed == true){
            System.print("Dashing")
            dashTimer = dashTimer + dt
            if(dashTimer > 200.0){
                hasDashed = false
                dashTimer = 0.0
            }
        }

    }
}
