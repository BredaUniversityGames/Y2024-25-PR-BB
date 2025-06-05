import "engine_api.wren" for Engine, Game, ECS, Entity, Vec3, Vec2, Quat, Math, TransformComponent, Input, SpawnEmitterFlagBits

class PlayerMovement{

    construct new(newHasDashed, newDashTimer, gun, playerController){
        hasDashed = newHasDashed
        hasDoubleJumped = false
        dashTimer = newDashTimer
        _gun = gun
        maxSpeed = 13.0
        sv_accelerate = 10.0
        jumpForce = 9.75
        gravityFactor = 2.4
        playerHeight = 1.7
        _cameraFovNormal = 50
        _cameraFovSlide = 65
        _cameraFovCurrent = _cameraFovNormal
        // Used for interpolation between crouching and standing
        currentPlayerHeight = playerHeight
        isGrounded = false
        _groundNormal = Vec3.new(0.0, 0.0, 0.0)
        isSliding = false
        slideForce = 3.0
        slideWishDirection = Vec3.new(0.0,0.0,0.0)

        var playerRotation = Math.ToEuler(playerController.GetTransformComponent().rotation)
        _cameraPitch = -playerRotation.y
        _cameraYaw = 0

        dashWishPosition = Vec3.new(0.0,0.0,0.0)
        dashForce = 6.0
        currentDashCount = 3
        currentDashRefillTime = 3000.0

        _lookSensitivity = 1.0
        _freeCamSpeedMultiplier = 0.1
        _smoothedCameraDelta = Vec2.new(0.0,0.0)
        _lastMousePosition = Vec2.new(0.0 ,0.0)

        _slideSoundInstance = null
        _crowsSoundInstance = null

        _slideSFX = "event:/SFX/Slide"
        _outofBounds = "event:/SFX/Crows"

        _maxStepDelay = 400
        _stepTimer = 0
        _walkSFX = "event:/SFX/Walk"
        _jumpSFX = "event:/SFX/Jump"
        _dashSFX = "event:/SFX/Dash"
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
    dashWishPosition {_dashWishPosition}
    currentDashCount {_currentDashCount}
    currentDashRefillTime {_currentDashRefillTime}

    //Base movement
    maxSpeed {_maxSpeed}
    sv_accelerate {_sv_accelerate}
    jumpForce {_jumpForce}
    gravityFactor {_gravityFactor} // A multiplier for the regular 9.8 gravity of the physics simulation
    playerHeight {_playerHeight}
    currentPlayerHeight {_currentPlayerHeight}
    isGrounded {_isGrounded}
    groundNormal {_groundNormal}

    // Input
    lastMousePosition {_lastMousePosition}
    lookSensitivity {_lookSensitivity}




//setters
    //Extra movement
    hasDashed=(value) { _hasDashed  = value}
    hasDoubleJumped=(value) { _hasDoubleJumped = value}
    dashTimer=(value) { _dashTimer = value}
    isSliding=(value) { _isSliding = value}
    slideWishDirection=(value) { _slideWishDirection = value}
    slideForce=(value) { _slideForce = value}
    dashForce=(value) { _dashForce = value}
    dashWishPosition=(value) { _dashWishPosition = value}
    currentDashCount=(value) { _currentDashCount = value}
    currentDashRefillTime=(value) { _currentDashRefillTime = value}

    //Base movement
    maxSpeed=(value) { _maxSpeed = value}
    sv_accelerate=(value) { _sv_accelerate = value}
    jumpForce=(value) { _jumpForce = value}
    gravityFactor=(value) { _gravityFactor = value}
    playerHeight=(value) { _playerHeight = value}
    currentPlayerHeight=(value) { _currentPlayerHeight = value}
    isGrounded=(value) { _isGrounded = value}
    groundNormal=(value) { _groundNormal = value}

    // Input
    lastMousePosition=(value) {_lastMousePosition = value}
    lookSensitivity=(value) {_lookSensitivity = value}

    Rotation(engine, player) {

        var dt = engine.GetTime().GetDeltatime()
        var FORWARD = Vec3.new(0.0, 0.0, -1.0)

        var MOUSE_SENSITIVITY = 0.003 * _lookSensitivity
        var GAMEPAD_LOOK_SENSITIVITY = 0.0025 * _lookSensitivity

        var mousePosition = engine.GetInput().GetMousePosition()
        var mouseDelta = _lastMousePosition - mousePosition
        var rotationDelta = Vec2.new(-mouseDelta.x * MOUSE_SENSITIVITY, mouseDelta.y * MOUSE_SENSITIVITY)


        var lookAnalogAction = engine.GetInput().GetAnalogAction("Look")
        rotationDelta.x = rotationDelta.x + lookAnalogAction.x * GAMEPAD_LOOK_SENSITIVITY * dt
        rotationDelta.y = rotationDelta.y + lookAnalogAction.y * GAMEPAD_LOOK_SENSITIVITY * dt

        _cameraPitch = _cameraPitch + rotationDelta.x
        _cameraYaw = Math.Min(Math.Max(_cameraYaw + rotationDelta.y, Math.Radians(-90.0)), Math.Radians(90.0))
        var forwardVector = Math.ToQuat(Vec3.new(_cameraYaw, -_cameraPitch, 0))

        var gun = engine.GetECS().GetEntityByName("Gun")
        var gunTransform = gun.GetTransformComponent()

        var gun2 = engine.GetECS().GetEntityByName("Gun2")
        var gunTransform2 = gun2.GetTransformComponent()

        var lerpFactor = 0.97
        var divisionFactor = 1.1
           var max = 0.6
        _smoothedCameraDelta.x = (_smoothedCameraDelta.x * lerpFactor +  rotationDelta.x * (1-lerpFactor)) / divisionFactor
        _smoothedCameraDelta.y = (_smoothedCameraDelta.y * lerpFactor +  rotationDelta.y * (1-lerpFactor)) / divisionFactor

        var clampedX  = Math.Clamp(_smoothedCameraDelta.x*5,-max,max)
        var clampedY  = Math.Clamp(_smoothedCameraDelta.y*5,-max,max)

        gunTransform.translation = Vec3.new(clampedX,clampedY,0)
        gunTransform.rotation = Math.ToQuat(Vec3.new(0,-Math.PI()/2+clampedX,clampedY*0.2))

        gunTransform2.translation = Vec3.new(clampedX,clampedY,0)
        gunTransform2.rotation = Math.ToQuat(Vec3.new(0,-Math.PI()/2+clampedX,clampedY*0.2)) 


        player.GetTransformComponent().rotation = forwardVector
    }

    FlyCamMovement(engine, player) {

        var rotation = player.GetTransformComponent().GetWorldRotation()

        var forward = Math.ToVector(rotation)
        var up = rotation.mulVec3(Vec3.new(0, 1, 0))
        var right = Math.Cross(forward, up)

        var CAM_SPEED = _freeCamSpeedMultiplier

        var movementDir = Vec3.new(0.0, 0.0, 0.0)
        var analogMovement = engine.GetInput().GetAnalogAction("Move")

        movementDir = movementDir + right.mulScalar(analogMovement.x)
        movementDir = movementDir + forward.mulScalar(analogMovement.y)

        if (movementDir.length() != 0) {
            movementDir = movementDir.normalize()
        }

        var position = player.GetTransformComponent().translation
        var scaled = CAM_SPEED

        position = position + movementDir.mulScalar(scaled)

        player.GetTransformComponent().translation = position
    }

    Movement(engine, playerController, camera) {

        var cheats = playerController.GetCheatsComponent()
        if(cheats.noClip && !engine.IsDistribution()) {
            this.Rotation(engine, engine.GetECS().GetEntityByName("Player"))
            this.FlyCamMovement(engine, engine.GetECS().GetEntityByName("Player"))
            return
        }

        if (engine.GetTime().GetDeltatime() != 0.0) {
            this.Rotation(engine, engine.GetECS().GetEntityByName("Player"))
        }

        var playerBody = playerController.GetRigidbodyComponent()
        var velocity = playerBody.GetVelocity()

        var cameraRotation = camera.GetTransformComponent().GetWorldRotation()
        var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
        forward.y = 0.0

        var right = (cameraRotation.mulVec3( Vec3.new(1.0, 0.0, 0.0))).normalize()

        // lets test for ground here
        var playerControllerPos = playerBody.GetPosition()
        var rayDirection = Vec3.new(0.0, -1.0, 0.0)
        var rayLength = 2.0

        var groundCheckRay = engine.GetPhysics().ShootRay(playerControllerPos, rayDirection, rayLength)
        var groundHitNormal = Vec3.new(0.0, 0.0, 0.0)

        isGrounded = false
        for(hit in groundCheckRay) {
            if(hit.GetEntity(engine.GetECS()) != playerController) {
                isGrounded = true
                groundHitNormal = hit.normal
                groundNormal = hit.normal // to be used outside the main movement function
                break
            }
        }

        var movement = engine.GetInput().GetAnalogAction("Move")
        var moveInputDir = Vec3.new(0.0,0.0,0.0)
        moveInputDir = forward.mulScalar(movement.y) + right.mulScalar(movement.x)
        moveInputDir = moveInputDir.normalize()

        if(moveInputDir.length() > 0.01){
            var dot = Math.Dot(moveInputDir, groundHitNormal)
            var moveProjected = moveInputDir - groundHitNormal.mulScalar(dot)
            moveInputDir = moveProjected.normalize()
        }

        if(movement.length() > 0.1){
            playerBody.SetFriction(0.5)
            _gun.playWalkAnim(engine)
        }else{
            playerBody.SetFriction(12.0)
            _gun.playIdleAnim(engine)
        }

        playerBody.SetGravityFactor(gravityFactor)

        var isJumpHeld = engine.GetInput().GetDigitalAction("Jump").IsHeld()
        var doubleJump = engine.GetInput().GetDigitalAction("Jump").IsPressed()

                

        if(isGrounded && isJumpHeld) {
            velocity.y = 0.0
            velocity = velocity + Vec3.new(0.0, jumpForce, 0.0)
            hasDoubleJumped = false

            //Jump sound
            var eventInstance = engine.GetAudio().PlayEventOnce(_jumpSFX)
            engine.GetAudio().SetEventVolume(eventInstance, 1.8)
            var audioEmitter = camera.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)
        }else {
            if(doubleJump && hasDoubleJumped == false){
                velocity.y = 0.0
                velocity = velocity + Vec3.new(0.0, jumpForce*1.5, 0.0)
                //Jump sound
                var eventInstance = engine.GetAudio().PlayEventOnce(_jumpSFX)
                engine.GetAudio().SetEventVolume(eventInstance, 1.8)
                var audioEmitter = camera.GetAudioEmitterComponent()
                audioEmitter.AddEvent(eventInstance)
                if(moveInputDir.length() > 0.01){
                    velocity = velocity + moveInputDir.mulScalar(maxSpeed/1.5)
                    
                }
                hasDoubleJumped = true
            }
        }

        var frameTime = engine.GetTime().GetDeltatime()
        var wishVel = moveInputDir.mulScalar(maxSpeed)

        //Fix for moving on slopes
        if(isGrounded && moveInputDir.length() > 0.01 && isJumpHeld == false){
            // Get the right vector relative to movement
            var lateral = Math.Cross(groundHitNormal, moveInputDir).normalize()

            // Remove velocity component in the lateral direction
            var latMag = Math.Dot(velocity, lateral)
            velocity = velocity - lateral.mulScalar(latMag)
        }


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

            //add slide here
            if(slideWishDirection.length() > 0.01){
                velocity = velocity + slideWishDirection.mulScalar(slideForce)
            }

            var speed = velocity.length()
            if (speed > maxSpeed) {
                var factor = maxSpeed / speed
                var newVel  =  Vec3.new(velocity.x * factor, velocity.y, velocity.z * factor)
                velocity = Math.MixVec3(velocity, newVel, 0.2)
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


        playerBody.SetVelocity(velocity)

        var pos = playerBody.GetPosition()

        pos.y = pos.y + currentPlayerHeight/2.0
        engine.GetECS().GetEntityByName("Player").GetTransformComponent().translation = pos
        // Play walk sound

        if(isGrounded){
            if(_stepTimer < _maxStepDelay){
                _stepTimer = _stepTimer + engine.GetTime().GetDeltatime()
            }else{
                var player = engine.GetECS().GetEntityByName("Camera")
                var eventInstance = engine.GetAudio().PlayEventOnce(_walkSFX)
                var volume = Math.Clamp( 0.25 * velocity.length(), 0.0, 2.0)
                engine.GetAudio().SetEventVolume(eventInstance, volume)
                var audioEmitter = player.GetAudioEmitterComponent()
                audioEmitter.AddEvent(eventInstance)
                _stepTimer = 0.0
            }

        }

    }

    Dash(engine, dt, playerController, camera, hud){
        //refill dashes
        if(currentDashCount < 3){
            currentDashRefillTime = currentDashRefillTime - dt
            if(currentDashRefillTime <= 0.0){
                currentDashCount = currentDashCount + 1
                currentDashRefillTime = 3000.0
                hud.TriggerDashChargeRefillAnimation()
            }
        }

        var playerBody = playerController.GetRigidbodyComponent()
        if(engine.GetInput().GetDigitalAction("Dash").IsPressed() &&  currentDashCount > 0 ){

            hasDashed = true

            //play dash sound
            var eventInstance = engine.GetAudio().PlayEventOnce(_dashSFX)
            engine.GetAudio().SetEventVolume(eventInstance, 2.8)
            var audioEmitter = camera.GetAudioEmitterComponent()
            audioEmitter.AddEvent(eventInstance)

            var player = engine.GetECS().GetEntityByName("Camera")
            var translation = playerBody.GetPosition()
            var rotation = camera.GetTransformComponent().GetWorldRotation()
            var forward = Math.ToVector(rotation)
            var up = rotation.mulVec3(Vec3.new(0, 1, 0))
            var right = Math.Cross(forward, up)
            var movement = engine.GetInput().GetAnalogAction("Move")

            var moveInputDir = Vec3.new(0.0,0.0,0.0)
            moveInputDir = forward.mulScalar(movement.y) + right.mulScalar(movement.x)
            moveInputDir = moveInputDir.normalize()

            if(moveInputDir.length() > 0.01){
                forward  = forward + moveInputDir
            }else{
                moveInputDir = forward.mulScalar(1.0) + right.mulScalar(0.0)
                moveInputDir = moveInputDir.normalize()
                forward  = forward + moveInputDir

            }

            var start = translation + forward * Vec3.new(0.1, 0.1, 0.1) //- right * Vec3.new(0.09, 0.09, 0.09) //- up * Vec3.new(0.12, 0.12, 0.12)
            var end = translation + forward * Vec3.new(dashForce, dashForce,dashForce)
            // Calculate the raw dash vector
            var dashVector = end - start
            var dashLength = dashVector.length()

            var direction = Vec3.new(0.0, 0.0, 0.0)

            // Check if the dash vector has a non-zero length before normalizing
            if (dashLength > 0.001) {
                direction = dashVector.mulScalar(1.0 / dashLength) // Manual normalization to avoid normalize() on zero
                currentDashCount = currentDashCount - 1 // only decrement if dash is valid
            }

            // Offset a bit by the ground normal
            if(groundNormal.length() > 0.001 && isGrounded){
                direction = direction + groundNormal.mulScalar(0.1)
                direction = direction.normalize()
            }

            playerBody.SetVelocity(playerBody.GetVelocity() + direction.mulScalar(dashForce*15.0))



            // Keep using the raycast to determine end positions, it helps with cutting the dash and making it snappy
            var rayHitInfo = engine.GetPhysics().ShootRay(start, direction, dashForce)
            dashWishPosition = end
            if (!rayHitInfo.isEmpty) {
                for(hitInfo in rayHitInfo) {
                    if(hitInfo.GetEntity(engine.GetECS()) != playerController) {
                        end = hitInfo.position
                        //add some offset to the end position based on the normal
                        end = end + hitInfo.normal.mulScalar(1.5)
                        dashWishPosition = end
                        break
                    }
                }
            }
        }

         if(hasDashed){
            dashTimer = dashTimer + dt
            var velocity = playerBody.GetVelocity()
            var currentSpeed = velocity.length()

            if( currentSpeed > 50.0){
                var clampedVelocity = velocity.normalize().mulScalar(50.0)
                playerBody.SetVelocity(clampedVelocity)
            }

            if(Math.Distance(playerBody.GetPosition(), dashWishPosition) < 3.0){
                hasDashed = false
                dashTimer = 0.0
                playerBody.SetGravityFactor(gravityFactor)
                //clamp velocity at the end
                if( currentSpeed > maxSpeed){
                    var clampedVelocity = velocity.normalize().mulScalar(maxSpeed)
                    playerBody.SetVelocity(clampedVelocity)
                }

            }else{
                playerBody.SetGravityFactor(0.0)
                playerBody.SetFriction(99.0)
            }

            if(dashTimer > 200.0){
                hasDashed = false
                dashTimer = 0.0
                playerBody.SetGravityFactor(gravityFactor)
                //clamp velocity at the end
                if( currentSpeed > maxSpeed){
                    var clampedVelocity = velocity.normalize().mulScalar(maxSpeed)
                    playerBody.SetVelocity(clampedVelocity)
                }
            }
         }
    }

    Slide(engine, dt, playerController, camera){
        var isJumpHeld = engine.GetInput().GetDigitalAction("Jump").IsHeld()
        if(isJumpHeld){
            return
        }
        var slideAmount = slideForce
        if(engine.GetInput().GetDigitalAction("Slide").IsHeld()  && !hasDashed && false){
            isSliding = true
            //crouch first
            currentPlayerHeight = Math.MixFloat(currentPlayerHeight, playerHeight/4.0, 0.0035 * dt)
            engine.GetGame().AlterPlayerHeight(engine.GetPhysics(),engine.GetECS(),currentPlayerHeight)

            _cameraFovCurrent = Math.MixFloat(_cameraFovCurrent,_cameraFovSlide,0.2)
            camera.GetCameraComponent().fov = Math.Radians(_cameraFovCurrent)
            var playerBody = playerController.GetRigidbodyComponent()
            var velocity = playerBody.GetVelocity()

            var cameraRotation = camera.GetTransformComponent().GetWorldRotation()
            var forward = (Math.ToVector(cameraRotation)*Vec3.new(1.0, 0.0, 1.0)).normalize()
            forward.y = 0.0

            var right = cameraRotation.mulVec3(Vec3.new(1.0, 0.0, 0.0)).normalize()
            var movement = engine.GetInput().GetAnalogAction("Move")
            var moveInputDir = Vec3.new(0.0,0.0,0.0)
            moveInputDir = forward.mulScalar(movement.y) + right.mulScalar(movement.x)
            moveInputDir = moveInputDir.normalize()

            if(moveInputDir.length() > 0.01){
                slideWishDirection = Math.MixVec3(slideWishDirection, moveInputDir, 0.05)
            }



            playerBody.SetVelocity(velocity)

            //play slide sound
            if(!_slideSoundInstance && isGrounded){
                _slideSoundInstance = engine.GetAudio().PlayEventOnce(_slideSFX)
                camera.GetAudioEmitterComponent().AddEvent(_slideSoundInstance)
            }

        }else{
            if(_slideSoundInstance){
                engine.GetAudio().StopEvent(_slideSoundInstance)
                _slideSoundInstance = null
            }

            _cameraFovCurrent = Math.MixFloat(_cameraFovCurrent,_cameraFovNormal,0.2)
            camera.GetCameraComponent().fov = Math.Radians(_cameraFovCurrent)
            isSliding = false
            currentPlayerHeight = Math.MixFloat(currentPlayerHeight, playerHeight, 0.0035 * dt)
            engine.GetGame().AlterPlayerHeight(engine.GetPhysics(),engine.GetECS(),currentPlayerHeight)
            slideWishDirection = Math.MixVec3(slideWishDirection, Vec3.new(0.0,0.0,0.0), 0.05)
        }
    }


    CheckBounds(engine, playerController, camera){
        var currentPos = playerController.GetRigidbodyComponent().GetPosition()
        if(currentPos.y < -10.0){
            var playerBody = playerController.GetRigidbodyComponent()
            playerBody.SetVelocity(Vec3.new(0.0, 0.0, 0.0))
            playerBody.SetTranslation(Vec3.new(-27.0, 30.5, 7.0))

            //play a sound effect
            _crowsSoundInstance = engine.GetAudio().PlayEventOnce(_outofBounds)
            camera.GetAudioEmitterComponent().AddEvent(_crowsSoundInstance)

            //play a particle effect
            var entity = engine.GetECS().NewEntity()
            var transform = entity.AddTransformComponent()
            transform.translation = Vec3.new(-27.0, 27.5, 7.0)
            var lifetime = entity.AddLifetimeComponent()
            lifetime.lifetime = 2000.0
            var emitterFlags = SpawnEmitterFlagBits.eIsActive() | SpawnEmitterFlagBits.eSetCustomVelocity() // |
            engine.GetParticles().SpawnEmitter(entity, "Feathers",emitterFlags,Vec3.new(0.0, 0.0, 0.0),Vec3.new(0.0, 0.0, 0.0))

        }
    }

    Update(engine, dt, playerController, camera,hud){
        this.Movement(engine, playerController, camera)
        this.Dash(engine, dt, playerController, camera,hud)
        // this.Slide(engine, dt, playerController, camera)
        this.CheckBounds(engine, playerController, camera)
    }
}
