#include "engine.hpp"

#include <glm/glm.hpp>
#include <implot/implot.h>
#include <stb/stb_image.h>

#include "application_module.hpp"
#include "components/directional_light_component.hpp"
#include "components/name_component.hpp"
#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs.hpp"
#include "editor.hpp"
#include "gbuffers.hpp"
#include "input_manager.hpp"
#include "model_loader.hpp"
#include "old_engine.hpp"
#include "particles/emitter_component.hpp"
#include "particles/particle_interface.hpp"
#include "particles/particle_util.hpp"
#include "physics_module.hpp"
#include "pipelines/debug_pipeline.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
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

    _ecs = std::make_shared<ECS>();

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();

    TransformHelpers::UnsubscribeToEvents(_ecs->registry);
    RelationshipHelpers::SubscribeToEvents(_ecs->registry);
    // modules
    _physicsModule = std::make_unique<PhysicsModule>();

    // systems
    _ecs->AddSystem<PhysicsSystem>(*_ecs, *_physicsModule);

    _scene = std::make_shared<SceneDescription>();
    rendererModule.SetScene(_scene);

    std::vector<std::string> modelPaths = {
        "assets/models/RiggedSimple.glb",
        // "assets/models/CathedralGLB_GLTF.glb",
        // "assets/models/Terrain/scene.gltf",
        // "assets/models/ABeautifulGame/ABeautifulGame.gltf",
        // "assets/models/MetalRoughSpheres.glb"
    };

    std::vector<Model> models = rendererModule.FrontLoadModels(modelPaths);
    std::vector<entt::entity> entities;

    SceneLoader sceneLoader {};
    for (const auto& model : models)
    {
        for (size_t i = 0; i < 1; i++)
        {
            for (size_t j = 0; j < 1; j++)
            {
                auto loadedEntities = sceneLoader.LoadModelIntoECSAsHierarchy(rendererModule.GetRenderer()->GetContext(), *_ecs, model);
                entities.insert(entities.end(), loadedEntities.begin(), loadedEntities.end());

                TransformHelpers::SetLocalPosition(_ecs->registry, loadedEntities[0], glm::vec3(i, 0.0f, j) * 2.0f);
            }
        }
    }

    // TransformHelpers::SetLocalRotation(_ecs->registry, entities[0], glm::angleAxis(glm::radians(45.0f), glm::vec3 { 0.0f, 1.0f, 0.0f }));
    // TransformHelpers::SetLocalPosition(_ecs->registry, entities[0], glm::vec3 { 10.0f, 0.0f, 10.f });

    // TransformHelpers::SetLocalScale(_ecs->registry, entities[1], glm::vec3 { 4.0f });
    // TransformHelpers::SetLocalPosition(_ecs->registry, entities[1], glm::vec3 { 106.0f, 14.0f, 145.0f });

    // TransformHelpers::SetLocalPosition(_ecs->registry, entities[2], glm::vec3 { 20.0f, 0.0f, 20.0f });

    _editor = std::make_unique<Editor>(_ecs, rendererModule.GetRenderer(), rendererModule.GetImGuiBackend());

    _scene->camera.position = glm::vec3 { 0.0f, 0.2f, 0.0f };
    _scene->camera.fov = glm::radians(45.0f);
    _scene->camera.nearPlane = 0.01f;
    _scene->camera.farPlane = 600.0f;

    // TODO: Once level saving is done, this should be deleted
    entt::entity lightEntity = _ecs->registry.create();
    _ecs->registry.emplace<NameComponent>(lightEntity, "Directional Light");
    _ecs->registry.emplace<TransformComponent>(lightEntity);
    _ecs->registry.emplace<DirectionalLightComponent>(lightEntity, glm::vec3(244.0f, 183.0f, 64.0f) / 255.0f * 4.0f);

    TransformHelpers::SetLocalPosition(_ecs->registry, lightEntity, glm::vec3(7.3f, 1.25f, 4.75f));
    TransformHelpers::SetLocalRotation(_ecs->registry, lightEntity, glm::quat(-0.29f, 0.06f, -0.93f, -0.19f));

    _lastFrameTime = std::chrono::high_resolution_clock::now();

    glm::ivec2 mousePos;
    applicationModule.GetInputManager().GetMousePosition(mousePos.x, mousePos.y);
    _lastMousePos = mousePos;

    bblog::info("Successfully initialized engine!");
    return ModuleTickOrder::eTick;
}

void OldEngine::Tick(Engine& engine)
{
    // update input
    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();
    auto& input = applicationModule.GetInputManager();

    ZoneNamed(zone, "");
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> deltaTime = currentFrameTime - _lastFrameTime;
    _lastFrameTime = currentFrameTime;
    float deltaTimeMS = deltaTime.count();

    // Update animations TODO: remove this later
    const auto view = _ecs->registry.view<TransformComponent, AnimationChannel>();
    for (auto entity : view)
    {
        auto& animation = view.get<AnimationChannel>(entity);

        animation.animation->Update(deltaTimeMS / 1000.0f, _frameIndex);

        if (animation.translation.has_value())
        {
            glm::vec3 position = animation.translation.value().Sample(animation.animation->time);

            TransformHelpers::SetLocalPosition(_ecs->registry, entity, position);
        }
        if (animation.rotation.has_value())
        {
            glm::quat rotation = animation.rotation.value().Sample(animation.animation->time);

            TransformHelpers::SetLocalRotation(_ecs->registry, entity, rotation);
        }
        if (animation.scaling.has_value())
        {
            glm::vec3 scale = animation.scaling.value().Sample(animation.animation->time);

            TransformHelpers::SetLocalScale(_ecs->registry, entity, scale);
        }
    }

    rendererModule.GetRenderer()->GetDebugPipeline().ClearLines();

    const auto debugView = _ecs->registry.view<JointComponent, RelationshipComponent, WorldMatrixComponent>();
    for (auto entity : debugView)
    {
        auto& relationship = debugView.get<RelationshipComponent>(entity);
        auto& transform = debugView.get<WorldMatrixComponent>(entity);

        glm::mat4 matrix = TransformHelpers::GetWorldMatrix(transform);
        glm::mat4 parentMatrix = TransformHelpers::GetWorldMatrix(_ecs->registry, relationship.parent);

        glm::vec3 position { matrix[3][0], matrix[3][1], matrix[3][2] };
        glm::vec3 parentPosition { parentMatrix[3][0], parentMatrix[3][1], parentMatrix[3][2] };

        rendererModule.GetRenderer()->GetDebugPipeline().AddLine(position, parentPosition);
    }

    // update physics
    _physicsModule->UpdatePhysicsEngine(deltaTimeMS);
    auto linesData = _physicsModule->debugRenderer->GetLinesData();
    auto persistentLinesData = _physicsModule->debugRenderer->GetPersistentLinesData();
    _physicsModule->debugRenderer->ClearLines();
    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(linesData);
    rendererModule.GetRenderer()->GetDebugPipeline().AddLines(persistentLinesData);

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

        glm::vec3& rotation = _scene->camera.eulerRotation;
        rotation.x -= mouseDelta.y * MOUSE_SENSITIVITY;
        rotation.y -= mouseDelta.x * MOUSE_SENSITIVITY;

        rotation.x = std::clamp(rotation.x, glm::radians(-90.0f), glm::radians(90.0f));

        glm::vec3 movementDir {};
        if (input.IsKeyHeld(KeyboardCode::eW))
        {
            movementDir -= FORWARD;
        }

        if (input.IsKeyHeld(KeyboardCode::eS))
        {
            movementDir += FORWARD;
        }

        if (input.IsKeyHeld(KeyboardCode::eD))
        {
            movementDir += RIGHT;
        }

        if (input.IsKeyHeld(KeyboardCode::eA))
        {
            movementDir -= RIGHT;
        }

        if (glm::length(movementDir) != 0.0f)
        {
            movementDir = glm::normalize(movementDir);
        }

        _scene->camera.position += glm::quat(_scene->camera.eulerRotation) * movementDir * deltaTimeMS * CAM_SPEED;
        JPH::RVec3Arg cameraPos = { _scene->camera.position.x, _scene->camera.position.y, _scene->camera.position.z };
        _physicsModule->debugRenderer->SetCameraPos(cameraPos);

        // shoot rays
        if (ImGui::IsKeyPressed(ImGuiKey_Space))
        {
            const glm::vec3 cameraDir = (glm::quat(_scene->camera.eulerRotation) * -FORWARD);
            const RayHitInfo hitInfo = _physicsModule->ShootRay(_scene->camera.position + glm::vec3(0.0001), glm::normalize(cameraDir), 5.0);

            std::cout << "Hit: " << hitInfo.hasHit << std::endl
                      << "Entity: " << static_cast<int>(hitInfo.entity) << std::endl
                      << "Position: " << hitInfo.position.x << ", " << hitInfo.position.y << ", " << hitInfo.position.z << std::endl
                      << "Fraction: " << hitInfo.hitFraction << std::endl;
        }
    }
    _lastMousePos = { mouseX, mouseY };
    _frameIndex++;

    if (input.IsKeyPressed(KeyboardCode::eESCAPE))
        engine.SetExit(0);

    if (input.IsKeyPressed(KeyboardCode::eP))
    {
        rendererModule.GetParticleInterface().SpawnEmitter(ParticleInterface::EmitterPreset::eTest);
        spdlog::info("Spawned emitter!");
    }

    _ecs->UpdateSystems(deltaTimeMS);
    _ecs->GetSystem<PhysicsSystem>().CleanUp();
    _ecs->RemovedDestroyed();
    _ecs->RenderSystems();

    JPH::BodyManager::DrawSettings drawSettings;
    _physicsModule->physicsSystem->DrawBodies(drawSettings, _physicsModule->debugRenderer);

    _editor->Draw(_performanceTracker, rendererModule.GetRenderer()->GetBloomSettings());

    rendererModule.GetRenderer()->Render(deltaTimeMS);

    _performanceTracker.Update();

    _physicsModule->debugRenderer->NextFrame();

    FrameMark;
}

void OldEngine::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _editor.reset();

    TransformHelpers::UnsubscribeToEvents(_ecs->registry);
    RelationshipHelpers::UnsubscribeToEvents(_ecs->registry);
    _ecs.reset();
}

OldEngine::OldEngine() = default;
OldEngine::~OldEngine() = default;
