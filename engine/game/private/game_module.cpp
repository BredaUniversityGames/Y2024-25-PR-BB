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
#include "file_io.hpp"
#include "fonts.hpp"
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
#include "steam_module.hpp"
#include "systems/lifetime_system.hpp"
#include "time_module.hpp"
#include "ui/ui_menus.hpp"
#include "ui_module.hpp"

ModuleTickOrder GameModule::Init(Engine& engine)
{
    auto& ECS = engine.GetModule<ECSModule>();
    ECS.AddSystem<LifetimeSystem>();

    GraphicsContext& graphicsContext = *engine.GetModule<RendererModule>().GetGraphicsContext();

    auto& viewport = engine.GetModule<UIModule>().GetViewport();
    const glm::uvec2 viewportSize = viewport.GetExtend();

    auto font = LoadFromFile("assets/fonts/BLOODROSE.ttf", 100, graphicsContext);

    if (auto versionFile = fileIO::OpenReadStream("version.txt"))
    {
        std::string gameVersionText = fileIO::DumpStreamIntoString(versionFile.value());
        viewport.AddElement(GameVersionVisualization::Create(graphicsContext, viewportSize, font, gameVersionText));
    }

    _mainMenu = viewport.AddElement(MainMenu::Create(graphicsContext, viewportSize, font));
    _hud = viewport.AddElement(HUD::Create(graphicsContext, viewportSize, font));
    _loadingScreen = viewport.AddElement(LoadingScreen::Create(graphicsContext, viewportSize, font));
    _pauseMenu = viewport.AddElement(PauseMenu::Create(graphicsContext, viewportSize, font));
    _gameOver = viewport.AddElement(GameOverMenu::Create(graphicsContext, viewportSize, font));
    _controlsMenu = viewport.AddElement(ControlsMenu::Create(graphicsContext, engine.GetModule<ApplicationModule>().GetActionManager(), viewportSize, font));

    _mainMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _hud.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _loadingScreen.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _pauseMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _gameOver.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    // _controlsMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;

    auto OpenDiscordURL = [&engine]()
    {
        bblog::info("Opening Discord LINK");
        auto& steam = engine.GetModule<SteamModule>();
        if (steam.Available())
        {
            steam.OpenSteamBrowser(DISCORD_URL);
        }
        else
        {
            engine.GetModule<ApplicationModule>().OpenExternalBrowser(DISCORD_URL);
        }
    };

    _mainMenu.lock()->openLinkButton.lock()->OnPress(Callback { OpenDiscordURL });

    auto& particleModule = engine.GetModule<ParticleModule>();
    particleModule.LoadEmitterPresets();

    engine.GetModule<ApplicationModule>().GetActionManager().SetGameActions(GAME_ACTIONS);
    return ModuleTickOrder::eTick;
}

void GameModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
}

void GameModule::SetMainMenuEnabled(bool val)
{
    if (auto lock = _mainMenu.lock())
    {
        lock->visibility = val ? UIElement::VisibilityState::eUpdatedAndVisible : UIElement::VisibilityState::eNotUpdatedAndInvisible;
    }
}

void GameModule::SetLoadingScreenEnabled(bool val)
{
    if (auto lock = _loadingScreen.lock())
    {
        lock->visibility = val ? UIElement::VisibilityState::eUpdatedAndVisible : UIElement::VisibilityState::eNotUpdatedAndInvisible;
    }
}

void GameModule::SetGameOverMenuEnabled(bool val)
{
    if (auto lock = _gameOver.lock())
    {
        lock->visibility = val ? UIElement::VisibilityState::eUpdatedAndVisible : UIElement::VisibilityState::eNotUpdatedAndInvisible;
    }
}

void GameModule::SetHUDEnabled(bool val)
{
    if (auto lock = _hud.lock())
    {
        lock->visibility = val ? UIElement::VisibilityState::eUpdatedAndVisible : UIElement::VisibilityState::eNotUpdatedAndInvisible;
    }
}

void GameModule::SetPauseMenuEnabled(bool val)
{
    if (auto lock = _pauseMenu.lock())
    {
        lock->visibility = val ? UIElement::VisibilityState::eUpdatedAndVisible : UIElement::VisibilityState::eNotUpdatedAndInvisible;
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

#if !DISTRBUTION
    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eH))
        applicationModule.SetMouseHidden(!applicationModule.GetMouseHidden());
#endif

    {
        // TODO!!! This can be directly handled by the debug renderer/physics
        ZoneNamedN(updateCamera, "Update Physics camera", true);

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

#if !DISTRBUTION
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
#endif

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
