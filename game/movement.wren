import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

class PlayerMovement{

    construct new(newHasDashed, newDashTimer){
        hasDashed = newHasDashed
        hasDoubleJumped = false
        dashTimer = newDashTimer

        maxSpeed = 7.0
        sv_accelerate = 5.5
        jumpForce = 8.20
        gravityFactor = 2.2
        playerHeight = 1.7
        isGrounded = false
        isSliding = false
        slideForce = 3.5
        dashForce = 22.0
        slideWishDirection = Vec3.new(0.0,0.0,0.0)
        
        
    }


//getters
    //Extra movement
    hasDashed {_hasDashed}
    hasDoubleJumped {_hasDoubleJumped}
    dashTimer {_dashTimer}
    isSliding {_isSliding}
    slideWishDirection {_slideWishDirection}
    slideForce {_slideForce}
    dashForce {_dashForce}

    //Base movement
    maxSpeed {_maxSpeed}
    sv_accelerate {_sv_accelerate}
    jumpForce {_jumpForce}
    gravityFactor {_gravityFactor} // A multiplier for the regular 9.8 gravity of the physics simulation
    playerHeight {_playerHeight}
    isGrounded {_isGrounded}
    




//setters
    //Extra movement
    hasDashed=(value) { _hasDashed  = value}
    hasDoubleJumped=(value) { _hasDoubleJumped = value}
    dashTimer=(value) { _dashTimer = value}
    isSliding=(value) { _isSliding = value}
    slideWishDirection=(value) { _slideWishDirection = value}   
    slideForce=(value) { _slideForce = value}
    dashForce=(value) { _dashForce = value}

    //Base movement
    maxSpeed=(value) { _maxSpeed = value}
    sv_accelerate=(value) { _sv_accelerate = value}
    jumpForce=(value) { _jumpForce = value}
    gravityFactor=(value) { _gravityFactor = value}
    playerHeight=(value) { _playerHeight = value}
    isGrounded=(value) { _isGrounded = value}



    Movement(engine, playerController, camera){

        var cheats = playerController.GetCheatsComponent()
        if(cheats.noClip == true){
            return
        }

        var playerBody = playerController.GetRigidbodyComponent()
        var velocity = engine.GetPhysics().GetVelocity(playerBody)

        var cameraRotation = camera.GetTransformComponent().rotation
        var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
        forward.y = 0.0

        var right = (cameraRotation.mul( Vec3.new(1.0, 0.0, 0.0))).normalize()

        // lets test for ground here
        var playerControllerPos = engine.GetPhysics().GetPosition(playerBody)
        var rayDirection = Vec3.new(0.0, -1.0, 0.0)
        var rayLength = 2.0

        var groundCheckRay = engine.GetPhysics().ShootRay(playerControllerPos, rayDirection, rayLength)

        isGrounded = false
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
        engine.GetPhysics().SetGravityFactor(playerBody,gravityFactor) 


        var isJumpHeld = engine.GetInput().GetDigitalAction("Jump").IsHeld()
        var doubleJump = engine.GetInput().GetDigitalAction("Jump").IsPressed()



        if(isGrounded && isJumpHeld) {
            velocity.y = 0.0
            velocity = velocity + Vec3.new(0.0, jumpForce, 0.0)
            hasDoubleJumped = false
        }else {
            if(doubleJump && hasDoubleJumped == false){
                velocity.y = 0.0
                velocity = velocity + Vec3.new(0.0, jumpForce, 0.0)
                hasDoubleJumped = true
            }
        }

        var frameTime = engine.GetTime().GetDeltatime()
        var wishVel = moveInputDir * Vec3.new(maxSpeed,maxSpeed,maxSpeed)

        if(isGrounded && !hasDashed){

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
        // Somehow it works without it currently, leave it here we might need it later

        // // Wall Collision Fix - Project velocity if collision occurs
        // var wallCheckRays = engine.GetPhysics().ShootMultipleRays(playerControllerPos, velocity,1.25,3,35.0)

        // for(hit in wallCheckRays) {
        // if(hit.GetEntity(engine.GetECS()).GetEnttEntity() != playerController.GetEnttEntity()) {
        //     var wallNormal = hit.normal

        //     //Clip velocity
        //     var backoff = Math.Dot(velocity, wallNormal) * 1.0
        //     velocity = velocity - wallNormal *  Vec3.new(backoff,backoff,backoff)
        // }
        // }

        engine.GetPhysics().SetVelocity(playerBody, velocity)
        var pos = engine.GetPhysics().GetPosition(playerBody)
        if(isSliding == true){
            pos.y = pos.y + (playerHeight/4.0)/2.0
        }else{
            pos.y = pos.y + playerHeight/2.0
        }
        camera.GetTransformComponent().translation = pos
    }

    Dash(engine, dt, playerController, camera){
        var playerBody = playerController.GetRigidbodyComponent()
        var velocity = engine.GetPhysics().GetVelocity(playerBody)
        var dashAmount = dashForce
        if(isGrounded==false){
            dashAmount = dashForce/1.6
        }else{
            dashAmount = dashForce
        }
        if(engine.GetInput().GetDigitalAction("Dash").IsPressed()){

            hasDashed = true
            var cameraRotation = camera.GetTransformComponent().rotation
            var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
            forward.y = 0.0
            var right = (cameraRotation.mul( Vec3.new(1.0, 0.0, 0.0))).normalize()
            var movement = engine.GetInput().GetAnalogAction("Move")

            var moveInputDir = Vec3.new(0.0,0.0,0.0)
            moveInputDir = forward * Vec3.new(movement.y,movement.y,movement.y) + right * Vec3.new(movement.x,movement.x,movement.x)
            moveInputDir = moveInputDir.normalize()

            if(moveInputDir.length() > 0.01){
                velocity = velocity + (moveInputDir * Vec3.new(dashAmount,dashAmount,dashAmount))
                engine.GetPhysics().SetVelocity(playerBody, velocity)
            }else{
                velocity = velocity + (forward*Vec3.new(2.0,2.0,2.0)* Vec3.new(dashAmount,dashAmount,dashAmount))
                engine.GetPhysics().SetVelocity(playerBody, velocity)
            }
        }

        if(hasDashed){
            dashTimer = dashTimer + dt
            if(dashTimer > 200.0){
                hasDashed = false
                dashTimer = 0.0
            }
        }

    }

    Slide(engine, dt, playerController, camera){
        var slideAmount = slideForce * dt
        if(engine.GetInput().GetDigitalAction("Slide").IsHeld() && isGrounded && !hasDashed){
            isSliding = true
            //crouch first
            engine.GetGame().AlterPlayerHeight(engine.GetPhysics(),engine.GetECS(),playerHeight/4.0)
            var playerBody = playerController.GetRigidbodyComponent()
            var velocity = engine.GetPhysics().GetVelocity(playerBody)
            var cameraRotation = camera.GetTransformComponent().rotation
            var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
            forward.y = 0.0
            var right = (cameraRotation.mul( Vec3.new(1.0, 0.0, 0.0))).normalize()
            var movement = engine.GetInput().GetAnalogAction("Move")
            var moveInputDir = Vec3.new(0.0,0.0,0.0)
            moveInputDir = forward * Vec3.new(movement.y,movement.y,movement.y) + right * Vec3.new(movement.x,movement.x,movement.x)
            moveInputDir = moveInputDir.normalize()

            if(moveInputDir.length() > 0.01){
                slideWishDirection = moveInputDir
            }

            if(slideWishDirection.length() > 0.01){
                velocity = velocity + (slideWishDirection* Vec3.new(slideAmount,slideAmount,slideAmount))
            }
            engine.GetPhysics().SetVelocity(playerBody, velocity)

        }else{
            isSliding = false
            engine.GetGame().AlterPlayerHeight(engine.GetPhysics(),engine.GetECS(),playerHeight)
            slideWishDirection = Vec3.new(0.0,0.0,0.0)
        }
    }


    
    Update(engine, dt, playerController, camera){
        this.Movement(engine, playerController, camera)
        this.Dash(engine, dt, playerController, camera)
        this.Slide(engine, dt, playerController, camera)
    }
}
