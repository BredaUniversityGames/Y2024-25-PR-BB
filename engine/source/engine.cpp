#include "engine.hpp"

#include <glm/glm.hpp>
#include <implot/implot.h>

#include "application_module.hpp"
#include "audio_module.hpp"
#include "components/camera_component.hpp"
#include "components/directional_light_component.hpp"
#include "components/name_component.hpp"
#include "components/point_light_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "editor.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "input/input_manager.hpp"
#include "model_loader.hpp"
#include "old_engine.hpp"
#include "particle_module.hpp"
#include "physics_module.hpp"
#include "pipelines/debug_pipeline.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "resource_management/model_resource_manager.hpp"
#include "scene_loader.hpp"
#include "systems/physics_system.hpp"

ModuleTickOrder OldEngine::Init(Engine& engine)
{
    auto path = std::filesystem::current_path();
    spdlog::info("Current path: {}", path.string());

    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    spdlog::info("Starting engine...");

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();
    auto& particleModule = engine.GetModule<ParticleModule>();
    auto& audioModule = engine.GetModule<AudioModule>();

    std::vector<std::string> modelPaths = {
        "assets/models/BrainStem.glb",
        //"assets/models/Cathedral.glb",
        //"assets/models/Adventure.glb",
        "assets/models/DamagedHelmet.glb",
        //"assets/models/CathedralGLB_GLTF.glb",
        // "assets/models/Terrain/scene.gltf",
        //"assets/models/ABeautifulGame/ABeautifulGame.gltf",
        //"assets/models/MetalRoughSpheres.glb"
    };

    particleModule.LoadEmitterPresets();

    auto models = rendererModule.FrontLoadModels(modelPaths);
    std::vector<entt::entity> entities;

    auto modelResourceManager = rendererModule.GetRenderer()->GetContext()->Resources()->ModelResourceManager();

    _ecs = &engine.GetModule<ECSModule>();

    for (size_t i = 0; i < 5; i++)
    {
        for (size_t j = 0; j < 1; j++)
        {
            auto entity = SceneLoading::LoadModelIntoECSAsHierarchy(*_ecs, *modelResourceManager.Access(models[0].second), models[0].first.hierarchy, models[0].first.animation);
            entities.emplace_back(entity);

            TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), entity, glm::vec3(i, 0.0f, j) * 2.0f);
        }
    }

    SceneLoading::LoadModelIntoECSAsHierarchy(*_ecs, *modelResourceManager.Access(models[1].second), models[1].first.hierarchy, models[1].first.animation);
    // TransformHelpers::SetLocalScale(_ecs->GetRegistry(), env, glm::vec3 { 0.25f });

    _editor = std::make_unique<Editor>(*_ecs, rendererModule.GetRenderer(), rendererModule.GetImGuiBackend());

    // TODO: Once level saving is done, this should be deleted
    entt::entity lightEntity = _ecs->GetRegistry().create();
    _ecs->GetRegistry().emplace<NameComponent>(lightEntity, "Directional Light");
    _ecs->GetRegistry().emplace<TransformComponent>(lightEntity);

    DirectionalLightComponent& directionalLightComponent = _ecs->GetRegistry().emplace<DirectionalLightComponent>(lightEntity);
    directionalLightComponent.color = glm::vec3(244.0f, 183.0f, 64.0f) / 255.0f * 4.0f;

    TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), lightEntity, glm::vec3(7.3f, 1.25f, 4.75f));
    TransformHelpers::SetLocalRotation(_ecs->GetRegistry(), lightEntity, glm::quat(-0.29f, 0.06f, -0.93f, -0.19f));

    entt::entity cameraEntity = _ecs->GetRegistry().create();
    _ecs->GetRegistry().emplace<NameComponent>(cameraEntity, "Camera");
    _ecs->GetRegistry().emplace<TransformComponent>(cameraEntity);

    CameraComponent& cameraComponent = _ecs->GetRegistry().emplace<CameraComponent>(cameraEntity);
    cameraComponent.projection = CameraComponent::Projection::ePerspective;
    cameraComponent.fov = 45.0f;
    cameraComponent.nearPlane = 0.01f;
    cameraComponent.farPlane = 600.0f;

    TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), cameraEntity, glm::vec3(0.0f, 1.0f, 0.0f));

    _lastFrameTime = std::chrono::high_resolution_clock::now();

    glm::ivec2 mousePos;
    applicationModule.GetInputManager().GetMousePosition(mousePos.x, mousePos.y);
    _lastMousePos = mousePos;

    _ecs->GetSystem<PhysicsSystem>()->InitializePhysicsColliders();
    BankInfo masterBank;
    masterBank.path = "assets/sounds/Master.bank";

    BankInfo stringBank;
    stringBank.path = "assets/sounds/Master.strings.bank";

    BankInfo bi;
    bi.path = "assets/sounds/SFX.bank";

    audioModule.LoadBank(masterBank);
    audioModule.LoadBank(stringBank);
    audioModule.LoadBank(bi);

    bblog::info("Successfully initialized engine!");
    return ModuleTickOrder::eTick;
}

void OldEngine::Tick(Engine& engine)
{
    // update input
    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();
    auto& input = applicationModule.GetInputManager();
    auto& physicsModule = engine.GetModule<PhysicsModule>();
    auto& particleModule = engine.GetModule<ParticleModule>();
    auto& audioModule = engine.GetModule<AudioModule>();
    physicsModule.debugRenderer->SetState(rendererModule.GetRenderer()->GetDebugPipeline().GetState());

    ZoneNamed(zone, "");
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> deltaTime = currentFrameTime - _lastFrameTime;
    _lastFrameTime = currentFrameTime;
    float deltaTimeMS = deltaTime.count();

    // update physics
    auto linesData = physicsModule.debugRenderer->GetLinesData();
    auto persistentLinesData = physicsModule.debugRenderer->GetPersistentLinesData();
    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(linesData);
    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(persistentLinesData);

    physicsModule.debugRenderer->ClearLines();

    // Slow down application when minimized.
    if (applicationModule.isMinimized())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(16ms);
        return;
    }

    int32_t mouseX, mouseY;
    input.GetMousePosition(mouseX, mouseY);

    if (input.IsKeyPressed(KeyboardCode::eH))
        applicationModule.SetMouseHidden(!applicationModule.GetMouseHidden());

    {
        ZoneNamedN(zone, "Update Camera", true);

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
            constexpr float CAM_SPEED = 0.003f;

            glm::ivec2 mouseDelta = glm::ivec2 { mouseX, mouseY } - _lastMousePos;
            glm::vec2 rotationDelta = { mouseDelta.x * MOUSE_SENSITIVITY, mouseDelta.y * MOUSE_SENSITIVITY };

            rotationDelta.x += input.GetGamepadAxis(GamepadAxis::eGAMEPAD_AXIS_RIGHTX) * GAMEPAD_LOOK_SENSITIVITY;
            rotationDelta.y += input.GetGamepadAxis(GamepadAxis::eGAMEPAD_AXIS_RIGHTY) * GAMEPAD_LOOK_SENSITIVITY;

            glm::quat rotation = TransformHelpers::GetLocalRotation(transformComponent);
            glm::vec3 eulerRotation = glm::eulerAngles(rotation);
            eulerRotation.x -= rotationDelta.y;

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
            if (input.IsKeyHeld(KeyboardCode::eW))
            {
                movementDir += FORWARD;
            }

            if (input.IsKeyHeld(KeyboardCode::eS))
            {
                movementDir -= FORWARD;
            }

            if (input.IsKeyHeld(KeyboardCode::eD))
            {
                movementDir += RIGHT;
            }

            if (input.IsKeyHeld(KeyboardCode::eA))
            {
                movementDir -= RIGHT;
            }

            movementDir += RIGHT * input.GetGamepadAxis(GamepadAxis::eGAMEPAD_AXIS_LEFTX);
            movementDir -= FORWARD * input.GetGamepadAxis(GamepadAxis::eGAMEPAD_AXIS_LEFTY);

            if (glm::length(movementDir) != 0.0f)
            {
                movementDir = glm::normalize(movementDir);
            }

            glm::vec3 position = TransformHelpers::GetLocalPosition(transformComponent);
            position += rotation * movementDir * deltaTimeMS * CAM_SPEED;
            TransformHelpers::SetLocalPosition(_ecs->GetRegistry(), entity, position);

            JPH::RVec3Arg cameraPos = { position.x, position.y, position.z };
            physicsModule.debugRenderer->SetCameraPos(cameraPos);

            // shoot rays
            if (ImGui::IsKeyPressed(ImGuiKey_Space))
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
            }
        }
    }

    _lastMousePos = { mouseX, mouseY };

    if (input.IsKeyPressed(KeyboardCode::eESCAPE))
        engine.SetExit(0);

    if (input.IsKeyPressed(KeyboardCode::eF1))
    {
        rendererModule.GetRenderer()->GetDebugPipeline().SetState(!rendererModule.GetRenderer()->GetDebugPipeline().GetState());
    }

    if (input.IsKeyPressed(KeyboardCode::e0))
    {
        entt::entity entity = _ecs->GetRegistry().create();
        RigidbodyComponent rb(*physicsModule.bodyInterface, entity, eSPHERE);

        NameComponent node;
        node.name = "Physics Entity";
        _ecs->GetRegistry().emplace<NameComponent>(entity, node);
        _ecs->GetRegistry().emplace<RigidbodyComponent>(entity, rb);
        physicsModule.bodyInterface->SetLinearVelocity(rb.bodyID, JPH::Vec3(1.0f, 0.5f, 0.9f));

        particleModule.SpawnEmitter(entity, EmitterPresetID::eTest, SpawnEmitterFlagBits::eIsActive);
    }

    static uint32_t eventId {};

    if (input.IsKeyPressed(KeyboardCode::eO))
    {
        eventId = audioModule.StartLoopingEvent("event:/Weapons/Machine Gun");
    }

    if (input.IsKeyReleased(KeyboardCode::eO))
    {
        audioModule.StopEvent(eventId);
    }

    JPH::BodyManager::DrawSettings drawSettings;
    physicsModule.physicsSystem->DrawBodies(drawSettings, physicsModule.debugRenderer);

    _editor->Draw(_performanceTracker, rendererModule.GetRenderer()->GetBloomSettings());

    rendererModule.GetRenderer()->Render(deltaTimeMS);

    _performanceTracker.Update();

    physicsModule.debugRenderer->NextFrame();

    FrameMark;
}

void OldEngine::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _editor.reset();
}

OldEngine::OldEngine() = default;
OldEngine::~OldEngine() = default;
