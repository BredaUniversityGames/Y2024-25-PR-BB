#include "engine.hpp"
#include "application_module.hpp"
#include "input_manager.hpp"
#include "old_engine.hpp"

#include "components/relationship_helpers.hpp"
#include "components/transform_helpers.hpp"
#include "ecs.hpp"
#include "editor.hpp"
#include "gbuffers.hpp"
#include "imgui_impl_vulkan.h"
#include "model_loader.hpp"
#include "modules/physics_module.hpp"
#include "pipelines/debug_pipeline.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "scene_loader.hpp"
#include "systems/physics_system.hpp"
#include "vulkan_helper.hpp"
#include <stb/stb_image.h>

#include "implot/implot.h"
#include "particles/particle_interface.hpp"
#include "particles/particle_util.hpp"
#include <imgui_impl_sdl3.h>

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

    _ecs = std::make_shared<ECS>();

    _renderer = std::make_unique<Renderer>(applicationModule, _ecs);

    ImGui_ImplSDL3_InitForVulkan(applicationModule.GetWindowHandle());

    TransformHelpers::UnsubscribeToEvents(_ecs->registry);
    RelationshipHelpers::SubscribeToEvents(_ecs->registry);

    _scene = std::make_shared<SceneDescription>();
    _renderer->_scene = _scene;

    std::vector<std::string> modelPaths = {
        "assets/models/DamagedHelmet.glb",
        "assets/models/ABeautifulGame/ABeautifulGame.gltf"
    };

    std::vector<Model> models = _renderer->FrontLoadModels(modelPaths);
    std::vector<entt::entity> entities;
    SceneLoader sceneLoader {};
    for (const auto& model : models)
    {
        auto loadedEntities = sceneLoader.LoadModelIntoECSAsHierarchy(_renderer->GetBrain(), *_ecs, model);
        entities.insert(entities.end(), loadedEntities.begin(), loadedEntities.end());
    }

    TransformHelpers::SetLocalScale(_ecs->registry, entities[1], glm::vec3 { 10.0f });

    _renderer->UpdateBindless();

    _editor = std::make_unique<Editor>(*_ecs, *_renderer);

    _scene->camera.position = glm::vec3 { 0.0f, 0.2f, 0.0f };
    _scene->camera.fov = glm::radians(45.0f);
    _scene->camera.nearPlane = 0.01f;
    _scene->camera.farPlane = 100.0f;

    _lastFrameTime = std::chrono::high_resolution_clock::now();

    glm::ivec2 mousePos;
    applicationModule.GetInputManager().GetMousePosition(mousePos.x, mousePos.y);
    _lastMousePos = mousePos;

    _particleInterface = std::make_unique<ParticleInterface>(*_ecs);

    // modules
    _physicsModule = std::make_unique<PhysicsModule>();

    // systems
    _ecs->AddSystem<PhysicsSystem>(*_ecs, *_physicsModule);

    bblog::info("Successfully initialized engine!");
    return ModuleTickOrder::eTick;
}

void OldEngine::Tick(Engine& engine)
{
    // update input
    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& input = applicationModule.GetInputManager();

    ZoneNamed(zone, "");
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> deltaTime = currentFrameTime - _lastFrameTime;
    _lastFrameTime = currentFrameTime;
    float deltaTimeMS = deltaTime.count();

    // update physics
    _physicsModule->UpdatePhysicsEngine(deltaTimeMS);
    auto linesData = _physicsModule->debugRenderer->GetLinesData();
    _renderer->_debugPipeline->ClearLines();
    _physicsModule->debugRenderer->ClearLines();
    _renderer->_debugPipeline->AddLines(linesData);

    // Slow down application when minimized.
    if (applicationModule.isMinimized())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(16ms);
        return;
    }

    int32_t mouseX, mouseY;
    input.GetMousePosition(mouseX, mouseY);

    auto windowSize = applicationModule.DisplaySize();
    _scene->camera.aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);

    if (input.IsKeyPressed(KeyboardCode::eH))
        applicationModule.SetMouseHidden(!applicationModule.GetMouseHidden());

    if (applicationModule.GetMouseHidden())
    {
        ZoneNamedN(zone, "Update Camera", true);

        glm::ivec2 mouseDelta = glm::ivec2 { mouseX, mouseY } - _lastMousePos;

        constexpr float MOUSE_SENSITIVITY = 0.003f;
        constexpr float CAM_SPEED = 0.003f;

        constexpr glm::vec3 RIGHT = { 1.0f, 0.0f, 0.0f };
        constexpr glm::vec3 FORWARD = { 0.0f, 0.0f, 1.0f };
        // constexpr glm::vec3 UP = { 0.0f, -1.0f, 0.0f };

        _scene->camera.eulerRotation.x -= mouseDelta.y * MOUSE_SENSITIVITY;
        _scene->camera.eulerRotation.y -= mouseDelta.x * MOUSE_SENSITIVITY;

        glm::vec3 movementDir {};
        if (input.IsKeyHeld(KeyboardCode::eW))
            movementDir -= FORWARD;

        if (input.IsKeyHeld(KeyboardCode::eS))
            movementDir += FORWARD;

        if (input.IsKeyHeld(KeyboardCode::eD))
            movementDir += RIGHT;

        if (input.IsKeyHeld(KeyboardCode::eA))
            movementDir -= RIGHT;

        if (glm::length(movementDir) != 0.0f)
        {
            movementDir = glm::normalize(movementDir);
        }

        _scene->camera.position += glm::quat(_scene->camera.eulerRotation) * movementDir * deltaTimeMS * CAM_SPEED;
        JPH::RVec3Arg cameraPos = { _scene->camera.position.x, _scene->camera.position.y, _scene->camera.position.z };
        _physicsModule->debugRenderer->SetCameraPos(cameraPos);
    }
    _lastMousePos = { mouseX, mouseY };

    if (input.IsKeyPressed(KeyboardCode::eESCAPE))
        engine.SetExit(0);

    if (input.IsKeyPressed(KeyboardCode::eP))
    {
        _particleInterface->SpawnEmitter(ParticleInterface::EmitterPreset::eTest);
        spdlog::info("Spawned emitter!");
    }

    _ecs->UpdateSystems(deltaTimeMS);
    _ecs->GetSystem<PhysicsSystem>().CleanUp();
    _ecs->RemovedDestroyed();
    _ecs->RenderSystems();

    JPH::BodyManager::DrawSettings drawSettings;
    _physicsModule->physicsSystem->DrawBodies(drawSettings, _physicsModule->debugRenderer);

    _editor->Draw(_performanceTracker, _renderer->_bloomSettings, *_scene);

    _renderer->Render(deltaTimeMS);

    _performanceTracker.Update();

    _physicsModule->debugRenderer->NextFrame();

    FrameMark;
}

void OldEngine::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _renderer->_brain.device.waitIdle();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    _editor.reset();
    _renderer.reset();

    TransformHelpers::UnsubscribeToEvents(_ecs->registry);
    RelationshipHelpers::UnsubscribeToEvents(_ecs->registry);
    _ecs.reset();
}

OldEngine::OldEngine() = default;
OldEngine::~OldEngine() = default;
