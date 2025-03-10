#include "game_module.hpp"
#include "application_module.hpp"
#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "canvas.hpp"
#include "cheats_component.hpp"
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
#include "input/action_manager.hpp"
#include "input/input_device_manager.hpp"
#include "model_loading.hpp"
#include "particle_module.hpp"
#include "passes/debug_pass.hpp"
#include "passes/shadow_pass.hpp"
#include "pathfinding_module.hpp"
#include "physics_module.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "scene/scene_loader.hpp"
#include "scripting_module.hpp"
#include "systems/lifetime_system.hpp"
#include "time_module.hpp"
#include "ui/ui_menus.hpp"
#include "ui_module.hpp"

#include <components/static_mesh_component.hpp>

ModuleTickOrder GameModule::Init(Engine& engine)
{
    auto& ECS = engine.GetModule<ECSModule>();
    ECS.AddSystem<LifetimeSystem>();

    auto hud = HudCreate(*engine.GetModule<RendererModule>().GetGraphicsContext(), engine.GetModule<UIModule>().GetViewport().GetExtend());
    _hud = hud.second;
    engine.GetModule<UIModule>().GetViewport().AddElement<Canvas>(std::move(hud.first));

    auto path = std::filesystem::current_path();
    spdlog::info("Current path: {}", path.string());
    spdlog::info("Starting engine...");

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& particleModule = engine.GetModule<ParticleModule>();
    particleModule.LoadEmitterPresets();

    std::vector<std::string> modelPaths = {
        //"assets/models/Cathedral.glb",
        //"assets/models/AnimatedRifle.glb",
        //"assets/models/BrainStem.glb",
        //"assets/models/Adventure.glb",
        //"assets/models/DamagedHelmet.glb",
        //"assets/models/CathedralGLB_GLTF.glb",
        //"assets/models/Terrain/scene.gltf",
        //"assets/models/ABeautifulGame/ABeautifulGame.gltf",
        //"assets/models/MetalRoughSpheres.glb",
        //"assets/models/monkey.gltf",
    };
    auto entities = SceneLoading::LoadModels(engine, modelPaths);
    // auto gunEntity = entities[1];

    entt::entity cameraEntity = ECS.GetRegistry().create();
    ECS.GetRegistry().emplace<NameComponent>(cameraEntity, "Camera");
    ECS.GetRegistry().emplace<TransformComponent>(cameraEntity);
    ECS.GetRegistry().emplace<RelationshipComponent>(cameraEntity);

    // RelationshipHelpers::AttachChild(ECS.GetRegistry(), cameraEntity, gunEntity);

    CameraComponent& cameraComponent = ECS.GetRegistry().emplace<CameraComponent>(cameraEntity);
    cameraComponent.projection = CameraComponent::Projection::ePerspective;
    cameraComponent.fov = 45.0f;
    cameraComponent.nearPlane = 0.5f;
    cameraComponent.farPlane = 600.0f;
    cameraComponent.reversedZ = true;

    ECS.GetRegistry().emplace<AudioListenerComponent>(cameraEntity);

    glm::ivec2 mousePos;
    applicationModule.GetInputDeviceManager().GetMousePosition(mousePos.x, mousePos.y);
    _lastMousePos = mousePos;

    applicationModule.GetActionManager().SetGameActions(GAME_ACTIONS);
    bblog::info("Successfully initialized engine!");

    return ModuleTickOrder::eTick;
}

void GameModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
}

void GameModule::Tick(MAYBE_UNUSED Engine& engine)
{
    if (_updateHud == true)
    {
        float totalTime = engine.GetModule<TimeModule>().GetTotalTime().count();
        HudUpdate(_hud, totalTime);
    }

    auto& ECS = engine.GetModule<ECSModule>();

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();
    auto& inputDeviceManager = applicationModule.GetInputDeviceManager();
    auto& actionManager = applicationModule.GetActionManager();
    auto& physicsModule = engine.GetModule<PhysicsModule>();
    auto& audioModule = engine.GetModule<AudioModule>();
    auto& pathfindingModule = engine.GetModule<PathfindingModule>();
    auto& scriptingModule = engine.GetModule<ScriptingModule>();

    float deltaTimeMS = engine.GetModule<TimeModule>().GetDeltatime().count();

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

        auto cameraView = ECS.GetRegistry().view<CameraComponent, TransformComponent>();
        auto playerEntity = ECS.GetRegistry().view<PlayerTag>().front();
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
            TransformHelpers::SetLocalRotation(ECS.GetRegistry(), entity, rotation);

            glm::vec3 movementDir {};
            glm::vec2 moveAnalogAction = actionManager.GetAnalogAction("Move");
            movementDir += RIGHT * moveAnalogAction.x;
            movementDir += FORWARD * moveAnalogAction.y;

            if (glm::length(movementDir) != 0.0f)
            {
                movementDir = glm::normalize(movementDir);
            }

            glm::vec3 position = TransformHelpers::GetLocalPosition(transformComponent);
            position += rotation * movementDir * deltaTimeMS * CAM_SPEED;

            // Only update the position if the player is not in noclip mode
            if (ECS.GetRegistry().all_of<CheatsComponent>(playerEntity))
            {
                CheatsComponent& cheatsComponent = ECS.GetRegistry().get<CheatsComponent>(playerEntity);
                if (cheatsComponent.noClip == true)
                {
                    TransformHelpers::SetLocalPosition(ECS.GetRegistry(), entity, position);
                }
            }

            JPH::RVec3Arg cameraPos = { position.x, position.y, position.z };
            physicsModule._debugRenderer->SetCameraPos(cameraPos);
        }
    }

    _lastMousePos = { mouseX, mouseY };

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eESCAPE))
        engine.SetExit(0);

    // Toggle physics debug drawing
    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eF1))
    {
        physicsModule._debugRenderer->SetState(!physicsModule._debugRenderer->GetState());
    }

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eP))
        scriptingModule.SetMainScript(engine, "swap_test.wren");

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eO))
    {
        scriptingModule.SetMainScript(engine, "swap_test_2.wren");
    }

    // Toggle pathfinding debug drawing
    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eF2))
    {
        pathfindingModule.SetDebugDrawState(!pathfindingModule.GetDebugDrawState());
    }

    int8_t physicsDebugDrawing = physicsModule._debugRenderer->GetState(),
           pathfindingDebugDrawing = pathfindingModule.GetDebugDrawState();

    if (physicsDebugDrawing || pathfindingDebugDrawing)
    {
        rendererModule.GetRenderer()->GetDebugPipeline().SetState(true);
    }
    else
    {
        rendererModule.GetRenderer()->GetDebugPipeline().SetState(false);
    }

    if (physicsDebugDrawing)
    {
        auto linesData = physicsModule._debugRenderer->GetLinesData();
        auto persistentLinesData = physicsModule._debugRenderer->GetPersistentLinesData();
        physicsModule._debugRenderer->ClearLines();
        rendererModule.GetRenderer()->GetDebugPipeline().AddLines(linesData);
        rendererModule.GetRenderer()->GetDebugPipeline().AddLines(persistentLinesData);
    }

    if (pathfindingDebugDrawing)
    {
        // Update pathfinding module debug lines
        const std::vector<glm::vec3>& pathfindingLines = pathfindingModule.GetDebugLines();
        rendererModule.GetRenderer()->GetDebugPipeline().AddLines(pathfindingLines);
    }

    // TODO: Ability to toggle audio debug lines, right now they get rendered as soon as the debug renderer is enabled
    // Update audio module debug lines
    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(audioModule.GetDebugLines());
    audioModule.ClearLines();
}
