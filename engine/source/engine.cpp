#include "engine.hpp"

#include <implot/implot.h>
#include <stb/stb_image.h>

#include "application_module.hpp"
#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "components/camera_component.hpp"
#include "components/directional_light_component.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "game_actions.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "input/action_manager.hpp"
#include "input/input_device_manager.hpp"
#include "model_loader.hpp"
#include "old_engine.hpp"
#include "particle_module.hpp"
#include "particle_util.hpp"
#include "passes/debug_pass.hpp"
#include "physics_module.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "resource_management/model_resource_manager.hpp"
#include "scene_loader.hpp"
#include "systems/physics_system.hpp"
#include "time_module.hpp"

ModuleTickOrder OldEngine::Init(Engine& engine)
{
    auto path = std::filesystem::current_path();
    spdlog::info("Current path: {}", path.string());
    spdlog::info("Starting engine...");

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();
    auto& particleModule = engine.GetModule<ParticleModule>();
    auto& audioModule = engine.GetModule<AudioModule>();

    // modules

    std::vector<std::string> modelPaths = {
        //"assets/models/Cathedral.glb",
        "assets/models/parcour_test1.glb",
        "assets/models/AnimatedRifle.glb",
        //"assets/models/Cathedral.glb"
        //"assets/models/BrainStem.glb",
        //"assets/models/Adventure.glb",
        //"assets/models/DamagedHelmet.glb",
        //"assets/models/CathedralGLB_GLTF.glb",
        //"assets/models/Terrain/scene.gltf",
        //"assets/models/ABeautifulGame/ABeautifulGame.gltf",
        //"assets/models/MetalRoughSpheres.glb",
        //"assets/models/monkey.gltf",

    };

    particleModule.LoadEmitterPresets();

    auto models = rendererModule.FrontLoadModels(modelPaths);
    std::vector<entt::entity> entities;

    auto modelResourceManager = rendererModule.GetRenderer()->GetContext()->Resources()->ModelResourceManager();

    _ecs = &engine.GetModule<ECSModule>();

    SceneLoading::LoadModelIntoECSAsHierarchy(*_ecs, *modelResourceManager.Access(models[0].second), models[0].first, models[0].first.hierarchy, models[0].first.animations);
    auto gunEntity = SceneLoading::LoadModelIntoECSAsHierarchy(*_ecs, *modelResourceManager.Access(models[1].second), models[1].first, models[1].first.hierarchy, models[1].first.animations);

    // TransformHelpers::SetLocalScale(_ecs->GetRegistry(), entities[1], glm::vec3 { 4.0f });
    //  TransformHelpers::SetLocalScale(_ecs->GetRegistry(), entities[1], glm::vec3 { 4.0f });
    //  TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), entities[1], glm::vec3 { 106.0f, 14.0f, 145.0f });

    // TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), entities[2], glm::vec3 { 20.0f, 0.0f, 20.0f });

    // TODO: Once level saving is done, this should be deleted
    entt::entity lightEntity
        = _ecs->GetRegistry().create();
    _ecs->GetRegistry().emplace<NameComponent>(lightEntity, "Directional Light");
    _ecs->GetRegistry().emplace<TransformComponent>(lightEntity);

    DirectionalLightComponent& directionalLightComponent = _ecs->GetRegistry().emplace<DirectionalLightComponent>(lightEntity);
    directionalLightComponent.color = glm::vec3(244.0f, 183.0f, 64.0f) / 255.0f * 4.0f;
    directionalLightComponent.nearPlane = -110.0f;
    directionalLightComponent.farPlane = 63.0f;
    directionalLightComponent.orthographicSize = 75.0f;

    TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), lightEntity, glm::vec3(-3.6f, 1.25f, 240.0f));
    TransformHelpers::SetLocalRotation(_ecs->GetRegistry(), lightEntity, glm::quat(-0.29f, 0.06f, -0.93f, -0.19f));

    entt::entity cameraEntity = _ecs->GetRegistry().create();
    _ecs->GetRegistry().emplace<NameComponent>(cameraEntity, "Camera");
    _ecs->GetRegistry().emplace<TransformComponent>(cameraEntity);
    _ecs->GetRegistry().emplace<RelationshipComponent>(cameraEntity);

    RelationshipHelpers::AttachChild(_ecs->GetRegistry(), cameraEntity, gunEntity);

    CameraComponent& cameraComponent = _ecs->GetRegistry().emplace<CameraComponent>(cameraEntity);
    cameraComponent.projection = CameraComponent::Projection::ePerspective;
    cameraComponent.fov = 45.0f;
    cameraComponent.nearPlane = 0.5f;
    cameraComponent.farPlane = 600.0f;
    cameraComponent.reversedZ = true;

    _ecs->GetRegistry().emplace<AudioListenerComponent>(cameraEntity);

    glm::ivec2 mousePos;
    applicationModule.GetInputDeviceManager().GetMousePosition(mousePos.x, mousePos.y);
    _lastMousePos = mousePos;

    // COMMEMTED OUT TEMPORARELY BECAUSE OF PROBLEMS WITH THE COLLIDDER LOADING
    // auto* physics_system = _ecs->GetSystem<PhysicsSystem>();
    //  physics_system->InitializePhysicsColliders();

    // BankInfo masterBank;
    // masterBank.path = "assets/sounds/Master.bank";
    //
    // BankInfo stringBank;
    // stringBank.path = "assets/sounds/Master.strings.bank";
    //
    // BankInfo bi;
    // bi.path = "assets/sounds/SFX.bank";
    //
    // audioModule.LoadBank(masterBank);
    // audioModule.LoadBank(stringBank);
    // audioModule.LoadBank(bi);

    SoundInfo si;
    si.path = "assets/sounds/fallback.mp3";
    si.is3D = true;

    audioModule.LoadSFX(si);

    SoundInfo musicSi;
    musicSi.path = "assets/sounds/music1.wav";
    musicSi.isLoop = true;
    musicSi.is3D = true;

    // This sound might pop in because it starts playing before the engine is fully initialized
    auto instance = audioModule.PlaySFX(audioModule.LoadSFX(musicSi), 1.0f, false);

    auto audioEmitter = _ecs->GetRegistry().create();
    _ecs->GetRegistry().emplace<TransformComponent>(audioEmitter);
    auto& emitter = _ecs->GetRegistry().emplace<AudioEmitterComponent>(audioEmitter);

    emitter._soundIds.emplace_back(instance);

    SoundInfo eagleSi;
    eagleSi.path = "assets/sounds/eagle.mp3";

    audioModule.LoadSFX(eagleSi);

    applicationModule.GetActionManager().SetGameActions(GAME_ACTIONS);

    bblog::info("Successfully initialized engine!");

    return ModuleTickOrder::eTick;
}

void OldEngine::Tick(Engine& engine)
{
    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();
    auto& inputDeviceManager = applicationModule.GetInputDeviceManager();
    auto& actionManager = applicationModule.GetActionManager();
    auto& physicsModule = engine.GetModule<PhysicsModule>();
    auto& particleModule = engine.GetModule<ParticleModule>();
    auto& audioModule = engine.GetModule<AudioModule>();
    physicsModule.debugRenderer->SetState(rendererModule.GetRenderer()->GetDebugPipeline().GetState());

    float deltaTimeMS = engine.GetModule<TimeModule>().GetDeltatime().count();

    // update physics
    auto linesData = physicsModule.debugRenderer->GetLinesData();
    auto persistentLinesData = physicsModule.debugRenderer->GetPersistentLinesData();
    rendererModule.GetRenderer()->GetDebugPipeline().ClearLines();
    physicsModule.debugRenderer->ClearLines();
    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(linesData);
    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(persistentLinesData);

    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(audioModule.GetDebugLines());
    audioModule.ClearLines();

    // Slow down application when minimized.
    if (applicationModule.isMinimized())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(16ms);
        return;
    }

    int32_t mouseX, mouseY;
    inputDeviceManager.GetMousePosition(mouseX, mouseY);

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eH))
        applicationModule.SetMouseHidden(!applicationModule.GetMouseHidden());

    {
        ZoneNamedN(updateCamera, "Update Camera", true);
        auto cameraView = _ecs->GetRegistry().view<CameraComponent, TransformComponent>();

        for (const auto& [entity, cameraComponent, transformComponent] : cameraView.each())
        {
            auto windowSize = applicationModule.DisplaySize();
            cameraComponent.aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);

            if (!applicationModule.GetMouseHidden())
            {
                continue;
            }

            constexpr glm::vec3 RIGHT = { 1.0f, 0.0f, 0.0f };
            constexpr glm::vec3 FORWARD = { 0.0f, 0.0f, -1.0f };

            constexpr float MOUSE_SENSITIVITY = 0.003f;
            constexpr float GAMEPAD_LOOK_SENSITIVITY = 0.025f;
            constexpr float CAM_SPEED = 0.03f;

            glm::ivec2 mouseDelta = _lastMousePos - glm::ivec2 { mouseX, mouseY };
            glm::vec2 rotationDelta = { -mouseDelta.x * MOUSE_SENSITIVITY, mouseDelta.y * MOUSE_SENSITIVITY };

            glm::vec2 lookAnalogAction = actionManager.GetAnalogAction("Look");
            rotationDelta.x += lookAnalogAction.x * GAMEPAD_LOOK_SENSITIVITY;
            rotationDelta.y += lookAnalogAction.y * GAMEPAD_LOOK_SENSITIVITY;

            glm::quat rotation = TransformHelpers::GetLocalRotation(transformComponent);
            glm::vec3 eulerRotation = glm::eulerAngles(rotation);
            eulerRotation.x += rotationDelta.y;

            // At 90 or -90 degrees yaw rotation, pitch snaps to 90 or -90 when using clamp here
            // eulerRotation.x = std::clamp(eulerRotation.x, glm::radians(-90.0f), glm::radians(90.0f));

            glm::vec3 cameraForward = glm::normalize(rotation * FORWARD);
            if (cameraForward.z > 0.0f)
                eulerRotation.y += rotationDelta.x;
            else
                eulerRotation.y -= rotationDelta.x;

            rotation = glm::quat(eulerRotation);
            TransformHelpers::SetLocalRotation(_ecs->GetRegistry(), entity, rotation);

            glm::vec3 movementDir {};
            if (inputDeviceManager.IsKeyHeld(KeyboardCode::eW))
            {
                movementDir += FORWARD;
            }

            if (inputDeviceManager.IsKeyHeld(KeyboardCode::eS))
            {
                movementDir -= FORWARD;
            }

            if (inputDeviceManager.IsKeyHeld(KeyboardCode::eD))
            {
                movementDir += RIGHT;
            }

            if (inputDeviceManager.IsKeyHeld(KeyboardCode::eA))
            {
                movementDir -= RIGHT;
            }

            glm::vec2 moveAnalogAction = actionManager.GetAnalogAction("Move");
            movementDir += RIGHT * moveAnalogAction.x;
            movementDir += FORWARD * moveAnalogAction.y;

            if (glm::length(movementDir) != 0.0f)
            {
                movementDir = glm::normalize(movementDir);
            }

            glm::vec3 position = TransformHelpers::GetLocalPosition(transformComponent);
            position += rotation * movementDir * deltaTimeMS * CAM_SPEED;

            glm::vec3 wishDir = rotation * movementDir * deltaTimeMS * CAM_SPEED;
            TestPlayerMovement(engine, deltaTimeMS, wishDir);

            // TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), entity, position);

            JPH::RVec3Arg cameraPos = { position.x, position.y, position.z };
            physicsModule.debugRenderer->SetCameraPos(cameraPos);

            // shoot rays
            /*if (inputDeviceManager.IsKeyPressed(KeyboardCode::eSPACE))
            {
                const glm::vec3 cameraDir = (rotation * FORWARD);
                const RayHitInfo hitInfo = physicsModule.ShootRay(position + glm::vec3(0.0001), glm::normalize(cameraDir), 5.0);

                std::cout << "Hit: " << hitInfo.hasHit << std::endl
                          << "Entity: " << static_cast<int>(hitInfo.entity) << std::endl
                          << "Position: " << hitInfo.position.x << ", " << hitInfo.position.y << ", " << hitInfo.position.z << std::endl
                          << "Fraction: " << hitInfo.hitFraction << std::endl;

                if (_ecs->GetRegistry().all_of<RigidbodyComponent>(hitInfo.entity))
                {

                    RigidbodyComponent& rb = _ecs->GetRegistry().get<RigidbodyComponent>(hitInfo.entity);

                    if (physicsModule.bodyInterface->GetMotionType(rb.bodyID) == JPH::EMotionType::Dynamic)
                    {
                        JPH::Vec3 forceDirection = JPH::Vec3(cameraDir.x, cameraDir.y, cameraDir.z) * 2000000.0f;
                        physicsModule.bodyInterface->AddImpulse(rb.bodyID, forceDirection);
                    }

                    particleModule.SpawnEmitter(hitInfo.entity, EmitterPresetID::eTest, SpawnEmitterFlagBits::eEmitOnce | SpawnEmitterFlagBits::eSetCustomPosition, hitInfo.position);
                }
            }*/
        }
    }

    _lastMousePos = { mouseX, mouseY };

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eESCAPE))
        engine.SetExit(0);

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eL))
    {
        audioModule.PlaySFX(audioModule.GetSFX("assets/sounds/eagle.mp3"), 1.5f, false);
    }

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eF1))
    {
        rendererModule.GetRenderer()->GetDebugPipeline().SetState(!rendererModule.GetRenderer()->GetDebugPipeline().GetState());
    }

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::e0))
    {
        entt::entity entity = _ecs->GetRegistry().create();
        RigidbodyComponent rb(*physicsModule.bodyInterface, entity, eSPHERE);

        NameComponent node;
        node.name = "Physics Entity";
        _ecs->GetRegistry().emplace<NameComponent>(entity, node);
        _ecs->GetRegistry().emplace<TransformComponent>(entity);
        _ecs->GetRegistry().emplace<RigidbodyComponent>(entity, rb);
        auto& audioEmitter = _ecs->GetRegistry().emplace<AudioEmitterComponent>(entity);

        physicsModule.bodyInterface->SetLinearVelocity(rb.bodyID, JPH::Vec3(1.0f, 0.5f, 0.9f));

        particleModule.SpawnEmitter(entity, EmitterPresetID::eTest, SpawnEmitterFlagBits::eIsActive);
        audioEmitter._soundIds.emplace_back(audioModule.PlaySFX(audioModule.GetSFX("assets/sounds/fallback.mp3"), 1.0f, false));
    }

    JPH::BodyManager::DrawSettings drawSettings;

    if (physicsModule.debugRenderer->GetState())
        physicsModule.physicsSystem->DrawBodies(drawSettings, physicsModule.debugRenderer);

    physicsModule.debugRenderer->NextFrame();

    FrameMark;
}

void OldEngine::Shutdown(MAYBE_UNUSED Engine& engine)
{
}
void OldEngine::TestPlayerMovement(Engine& engine, float deltaTime, glm::vec3 inputDir)
{
    entt::entity playerEntity = _ecs->GetSystem<PhysicsSystem>()->_playerEntity;
    if (playerEntity == entt::null)
        return;

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& inputDeviceManager = applicationModule.GetInputDeviceManager();
    auto& physicsModule = engine.GetModule<PhysicsModule>();
    auto& ecs = engine.GetModule<ECSModule>();
    auto& actionManager = applicationModule.GetActionManager();

    JPH::BodyID bodyID = ecs.GetRegistry().get<RigidbodyComponent>(playerEntity).bodyID;
    JPH::Vec3 velocity = physicsModule.bodyInterface->GetLinearVelocity(bodyID);

    if (velocity.Length() > 0.0f)
        bblog::info("Speed info before: {}", velocity.Length());

    auto cameraView = _ecs->GetRegistry().view<CameraComponent, TransformComponent>();
    glm::quat cameraRotation;

    for (const auto& [entity, cameraComponent, transformComponent] : cameraView.each())
    {
        cameraRotation = TransformHelpers::GetLocalRotation(transformComponent);
        break;
    }

    // Modified forward vector calculation to prevent camera pitch from affecting movement
    glm::vec3 forward = glm::normalize(glm::vec3(cameraRotation * glm::vec3(0.0f, 0.0f, -1.0f)) * glm::vec3(1.0f, 0.0f, 1.0f));
    forward.y = 0.0f; // Prevent vertical influence
    // forward = glm::normalize(forward);

    glm::vec3 right = glm::normalize(cameraRotation * glm::vec3(1.0f, 0.0f, 0.0f));

    // lets test for ground here
    glm::vec3 playerPos = glm::vec3(physicsModule.bodyInterface->GetPosition(bodyID).GetX(),
        physicsModule.bodyInterface->GetPosition(bodyID).GetY(),
        physicsModule.bodyInterface->GetPosition(bodyID).GetZ());
    glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    float rayLength = 2.0f;

    RayHitInfo groundCheckRay = physicsModule.ShootRay(playerPos, rayDirection, rayLength);

    bool isGrounded = false;
    for (auto hit : groundCheckRay.hitEntities)
    {
        if (hit != playerEntity && groundCheckRay.hasHit)
        {
            isGrounded = true;
            break;
        }
    }

    glm::vec3 moveInputDir = glm::vec3(0.0f);
    if (inputDeviceManager.IsKeyHeld(KeyboardCode::eW))
        moveInputDir += forward;
    if (inputDeviceManager.IsKeyHeld(KeyboardCode::eS))
        moveInputDir -= forward;
    if (inputDeviceManager.IsKeyHeld(KeyboardCode::eA))
        moveInputDir -= right;
    if (inputDeviceManager.IsKeyHeld(KeyboardCode::eD))
        moveInputDir += right;
    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eSPACE) && isGrounded)
    {
        velocity += JPH::Vec3(0.0f, 8.20f, 0.0f);
    }

    /*if (glm::length(moveInputDir) > 0.0f)
        moveInputDir = glm::normalize(moveInputDir);*/

    const float maxSpeed = 6.80f;
    const float sv_accelerate = 10.0f;
    const float frameTime = deltaTime;

    glm::vec3 wishVel = moveInputDir * maxSpeed;

    if (isGrounded)
    {
        physicsModule.bodyInterface->SetFriction(bodyID, 12.0f);
        float currentSpeed = glm::dot(glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ()), moveInputDir);

        float addSpeed = maxSpeed - currentSpeed;
        if (addSpeed > 0)
        {
            float accelSpeed = sv_accelerate * frameTime * maxSpeed;
            if (accelSpeed > addSpeed)
                accelSpeed = addSpeed;

            velocity += accelSpeed * JPH::Vec3(moveInputDir.x, moveInputDir.y, moveInputDir.z);
        }
    }
    else
    {
        // to fix the normalize and length shit form id tech
        // physicsModule.bodyInterface->SetFriction(bodyID, 0.0f);

        // Air acceleration
        float wishspeed = glm::length(wishVel);
        wishVel = glm::normalize(wishVel);
        if (wishspeed > 3.0f)
            wishspeed = 3.0f;

        float currentspeed = glm::dot(glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ()), wishVel);
        float addspeed = wishspeed - currentspeed;
        if (addspeed > 0)
        {
            float accelspeed = maxSpeed * sv_accelerate * frameTime;
            if (accelspeed > addspeed)
                accelspeed = addspeed;

            velocity += JPH::Vec3(wishVel.x, wishVel.y, wishVel.z) * accelspeed;
        }
    }

    physicsModule.bodyInterface->SetLinearVelocity(bodyID, velocity);

    glm::vec3 pos = glm::vec3(physicsModule.bodyInterface->GetPosition(bodyID).GetX(),
        physicsModule.bodyInterface->GetPosition(bodyID).GetY(),
        physicsModule.bodyInterface->GetPosition(bodyID).GetZ());
    pos.y += 0.5f;

    for (const auto& [entity, cameraComponent, transformComponent] : cameraView.each())
    {
        TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), entity, pos);
    }

    if (velocity.Length() > 0.0f)
        bblog::info("Speed info after: {}", velocity.Length());
}

OldEngine::OldEngine() = default;
OldEngine::~OldEngine() = default;
