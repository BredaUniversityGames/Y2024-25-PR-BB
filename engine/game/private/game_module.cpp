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
#include "components/static_mesh_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "game_actions.hpp"
#include "graphics_context.hpp"
#include "input/action_manager.hpp"
#include "input/input_device_manager.hpp"
#include "input_glyphs.hpp"
#include "model_loading.hpp"
#include "particle_module.hpp"
#include "passes/debug_pass.hpp"
#include "passes/shadow_pass.hpp"
#include "pathfinding_module.hpp"
#include "physics_module.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "scene/model_loader.hpp"
#include "scripting_module.hpp"
#include "systems/lifetime_system.hpp"
#include "time_module.hpp"
#include "ui/ui_menus.hpp"
#include "ui_module.hpp"
#include "file_io.hpp"

ModuleTickOrder GameModule::Init(Engine& engine)
{
    auto& ECS = engine.GetModule<ECSModule>();
    ECS.AddSystem<LifetimeSystem>();

    GraphicsContext& graphicsContext = *engine.GetModule<RendererModule>().GetGraphicsContext();
    const glm::uvec2 viewportSize = engine.GetModule<UIModule>().GetViewport().GetExtend();

    std::optional<std::ifstream> versionFile = fileIO::OpenReadStream("version.txt");
    if (versionFile.has_value())
    {
        std::string gameVersionText = fileIO::DumpStreamIntoString(versionFile.value());
        _gameVersionVisualization = GameVersionVisualizationCreate(graphicsContext, viewportSize, gameVersionText);
        engine.GetModule<UIModule>().GetViewport().AddElement<Canvas>(_gameVersionVisualization.canvas);
    }

    _hud = HudCreate(graphicsContext, viewportSize);
    auto mainMenu = std::make_shared<MainMenu>(MainMenuCreate(*engine.GetModule<RendererModule>().GetGraphicsContext(), engine.GetModule<UIModule>().GetViewport().GetExtend()));
    engine.GetModule<UIModule>().uiInputContext.focusedUIElement = mainMenu->playButton;
    _mainMenu = mainMenu;
    engine.GetModule<UIModule>().GetViewport().AddElement<Canvas>(_hud.canvas);
    engine.GetModule<UIModule>().GetViewport().AddElement<Canvas>(mainMenu);
    _mainMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _hud.canvas->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;

    auto path = std::filesystem::current_path();
    spdlog::info("Current path: {}", path.string());
    spdlog::info("Starting engine...");

    auto& particleModule = engine.GetModule<ParticleModule>();
    particleModule.LoadEmitterPresets();

    engine.GetModule<ApplicationModule>().GetActionManager().SetGameActions(GAME_ACTIONS);

    bblog::info("Successfully initialized engine!");

    return ModuleTickOrder::eTick;
}

void GameModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
}

void GameModule::SetMainMenuEnabled(bool val)
{
    if (val)
    {
        _mainMenu.lock()->visibility = UIElement::VisibilityState::eUpdatedAndVisible;
    }
    else
    {
        _mainMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    }
}
void GameModule::SetHUDEnabled(bool val)
{
    if (val)
    {
        _hud.canvas->visibility = UIElement::VisibilityState::eUpdatedAndVisible;
    }
    else
    {
        _hud.canvas->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    }
}
void GameModule::TransitionScene(const std::string& scriptFile)
{
    _nextSceneToExecute = scriptFile;
}

void GameModule::Tick(MAYBE_UNUSED Engine& engine)
{
    if (!_nextSceneToExecute.empty())
    {
        engine.GetModule<ScriptingModule>().SetMainScript(engine, _nextSceneToExecute);
        engine.GetModule<TimeModule>().ResetTimer();
    }

    _nextSceneToExecute.clear();

    if (_updateHud == true)
    {
        float totalTime = engine.GetModule<TimeModule>().GetTotalTime().count();
        HudUpdate(_hud, totalTime);
    }

    auto& ECS = engine.GetModule<ECSModule>();

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& rendererModule = engine.GetModule<RendererModule>();
    auto& inputDeviceManager = applicationModule.GetInputDeviceManager();
    auto& physicsModule = engine.GetModule<PhysicsModule>();
    auto& audioModule = engine.GetModule<AudioModule>();
    auto& pathfindingModule = engine.GetModule<PathfindingModule>();

    // Slow down application when minimized.
    if (applicationModule.isMinimized())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(16ms);
        return;
    }

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eH))
        applicationModule.SetMouseHidden(!applicationModule.GetMouseHidden());

    {
        ZoneNamedN(updateCamera, "Update Camera", true);

        auto cameraView = ECS.GetRegistry().view<CameraComponent, TransformComponent>();
        for (const auto& [entity, cameraComponent, transformComponent] : cameraView.each())
        {
            auto windowSize = applicationModule.DisplaySize();
            cameraComponent.aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);

            if (!applicationModule.GetMouseHidden())
            {
                continue;
            }

            glm::vec3 position = TransformHelpers::GetWorldPosition(ECS.GetRegistry(), entity);

            JPH::RVec3Arg cameraPos = { position.x, position.y, position.z };
            physicsModule._debugRenderer->SetCameraPos(cameraPos);
        }
    }

    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eESCAPE))
        engine.SetExit(0);

    // Toggle physics debug drawing
    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eF1))
    {
        physicsModule._debugRenderer->SetState(!physicsModule._debugRenderer->GetState());
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
