import "engine_api.wren" for Engine, TimeModule, ECS, Entity, Vec3, Vec2, Quat, Math, AnimationControlComponent, TransformComponent, Input, Keycode, SpawnEmitterFlagBits, EmitterPresetID

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
        
        _lookSensitivity = 1.0
        _freeCamSpeedMultiplier = 1.0

        _lastMousePosition = Vec2.new(0.0 ,0.0)
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

    // Input
    lastMousePosition {_lastMousePosition}




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

    // Input
    lastMousePosition=(value) {_lastMousePosition = value}


    Rotation(engine, player) {
        
        var FORWARD = Vec3.new(0.0, 0.0, -1.0)

        var MOUSE_SENSITIVITY = 0.003 * _lookSensitivity
        var GAMEPAD_LOOK_SENSITIVITY = 0.025 * _lookSensitivity

        var mousePosition = engine.GetInput().GetMousePosition()
        var mouseDelta = _lastMousePosition - mousePosition
        var rotationDelta = Vec2.new(-mouseDelta.x * MOUSE_SENSITIVITY, mouseDelta.y * MOUSE_SENSITIVITY)

        var lookAnalogAction = engine.GetInput().GetAnalogAction("Look")
        rotationDelta.x = rotationDelta.x + lookAnalogAction.x * GAMEPAD_LOOK_SENSITIVITY
        rotationDelta.y = rotationDelta.y + lookAnalogAction.y * GAMEPAD_LOOK_SENSITIVITY

        var rotation = player.GetTransformComponent().rotation

        var euler = Math.ToEuler(rotation)
        euler.x = euler.x + rotationDelta.y

        var cameraForward = FORWARD.mulQuat(rotation).normalize()

        if (cameraForward.z > 0.0) {
            euler.y = euler.y + rotationDelta.x
        } else {
            euler.y = euler.y - rotationDelta.x
        }

        rotation = Math.ToQuat(euler)

        player.GetTransformComponent().rotation = rotation
    }

    FlyCamMovement(engine, player) {

        var rotation = player.GetTransformComponent().GetWorldRotation()

        var forward = Math.ToVector(rotation)
        var up = rotation.mulVec3(Vec3.new(0, 1, 0))
        var right = Math.Cross(forward, up)

        var CAM_SPEED = 0.03 * _freeCamSpeedMultiplier

        var movementDir = Vec3.new(0.0, 0.0, 0.0)
        var analogMovement = engine.GetInput().GetAnalogAction("Move")

        movementDir = movementDir + right.mulScalar(analogMovement.x)
        movementDir = movementDir + forward.mulScalar(analogMovement.y)

        if (movementDir.length() != 0) {
            movementDir = movementDir.normalize()
        }

        var position = player.GetTransformComponent().translation
        var scaled = engine.GetTime().GetDeltatime() * CAM_SPEED

        position = position + movementDir.mulScalar(scaled)

        // position = position + rotation.mulVec3(movementDir).mulScalar(scaled)

        player.GetTransformComponent().translation = position
    }

    Movement(engine, playerController, camera) {

        this.Rotation(engine, engine.GetECS().GetEntityByName("Player"))

        var cheats = playerController.GetCheatsComponent()
        if(cheats.noClip == true){
            this.FlyCamMovement(engine, engine.GetECS().GetEntityByName("Player"))
            return
        }

        var playerBody = playerController.GetRigidbodyComponent()
        var velocity = engine.GetPhysics().GetVelocity(playerBody)

        var cameraRotation = camera.GetTransformComponent().GetWorldRotation()
        var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
        forward.y = 0.0

        var right = (cameraRotation.mulVec3( Vec3.new(1.0, 0.0, 0.0))).normalize()

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
        moveInputDir = forward.mulScalar(movement.y) + right.mulScalar(movement.x)
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
        var wishVel = moveInputDir.mulScalar(maxSpeed)

        if(isGrounded && !hasDashed){

            var currentSpeed = Math.Dot(velocity, moveInputDir)

            var addSpeed = maxSpeed - currentSpeed
            if (addSpeed > 0) {
                var accelSpeed = sv_accelerate * frameTime * maxSpeed
                if (accelSpeed > addSpeed) {
                    accelSpeed = addSpeed
                }

                velocity = velocity + moveInputDir.mulScalar(accelSpeed)
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
                var accelSpeed = maxSpeed * sv_accelerate*frameTime
                if(accelSpeed > addSpeed){
                    accelSpeed = addSpeed
                }
                velocity = velocity + wishVel.mulScalar(accelSpeed)
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
        engine.GetECS().GetEntityByName("Player").GetTransformComponent().translation = pos
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
            var cameraRotation = camera.GetTransformComponent().GetWorldRotation()
            var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
            forward.y = 0.0
            var right = (cameraRotation.mulVec3(Vec3.new(1.0, 0.0, 0.0))).normalize()
            var movement = engine.GetInput().GetAnalogAction("Move")

            var moveInputDir = Vec3.new(0.0,0.0,0.0)
            moveInputDir = forward.mulScalar(movement.y) + right.mulScalar(movement.x)
            moveInputDir = moveInputDir.normalize()

            if(moveInputDir.length() > 0.01){
                velocity = velocity + moveInputDir.mulScalar(dashAmount)
                engine.GetPhysics().SetVelocity(playerBody, velocity)
            }else{
                velocity = velocity + forward.mulScalar(2.0 * dashAmount)
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
            var cameraRotation = camera.GetTransformComponent().GetWorldRotation()
            var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
            forward.y = 0.0
            var right = cameraRotation.mulVec3(Vec3.new(1.0, 0.0, 0.0)).normalize()
            var movement = engine.GetInput().GetAnalogAction("Move")
            var moveInputDir = Vec3.new(0.0,0.0,0.0)
            moveInputDir = forward.mulScalar(movement.y) + right.mulScalar(movement.x)
            moveInputDir = moveInputDir.normalize()

            if(moveInputDir.length() > 0.01){
                slideWishDirection = moveInputDir
            }

            if(slideWishDirection.length() > 0.01){
                velocity = velocity + slideWishDirection.mulScalar(slideAmount)
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
