#include "game_module.hpp"
#include "achievements.hpp"
#include "application_module.hpp"
#include "audio_module.hpp"
#include "canvas.hpp"
#include "components/camera_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "file_io.hpp"
#include "fonts.hpp"
#include "game_actions.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "input/action_manager.hpp"
#include "input/input_device_manager.hpp"
#include "particle_module.hpp"
#include "passes/debug_pass.hpp"
#include "passes/particle_pass.hpp"
#include "passes/shadow_pass.hpp"
#include "passes/tonemapping_pass.hpp"
#include "pathfinding_module.hpp"
#include "physics_module.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "scene/model_loader.hpp"
#include "scripting_module.hpp"
#include "steam_module.hpp"
#include "systems/lifetime_system.hpp"
#include "time_module.hpp"
#include "ui/ui_menus.hpp"
#include "ui_module.hpp"
#include "ui_text.hpp"

#include <magic_enum.hpp>

Achievement CreateAchievement(SteamAchievementEnum achievements)
{
    return Achievement { static_cast<int32_t>(achievements), magic_enum::enum_name(achievements) };
}

Stat CreateStat(SteamStatEnum stats, EStatTypes type)
{
    return Stat {
        static_cast<int32_t>(stats),
        type, magic_enum::enum_name(stats)
    };
}

ModuleTickOrder GameModule::Init(Engine& engine)
{
    _achievements = {
        CreateAchievement(SteamAchievementEnum::SKELETONS_KILLED_1),
        CreateAchievement(SteamAchievementEnum::EYES_KILLED_1),
        CreateAchievement(SteamAchievementEnum::BERSERKERS_KILLED_1),
        CreateAchievement(SteamAchievementEnum::SOULS_1),
        CreateAchievement(SteamAchievementEnum::NUGGET_1),
        CreateAchievement(SteamAchievementEnum::DIE_1),
        CreateAchievement(SteamAchievementEnum::RELIC_1),
    };

    _stats = {
        CreateStat(SteamStatEnum::SKELETONS_KILLED, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::EYES_KILLED, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::BERSERKERS_KILLED, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::WAVES_REACHED, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::SOULS_COLLECTED, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::GOLD_NUGGETS_COLLECTED, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::GOLD_CURRENCY_COLLECTED, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::ENEMIES_KILLED_WITH_RELIC, EStatTypes::STAT_INT),
        CreateStat(SteamStatEnum::TIMES_DIED, EStatTypes::STAT_INT),
    };

    ActionManager& actionManager = engine.GetModule<ApplicationModule>().GetActionManager();
    actionManager.SetGameActions(GAME_ACTIONS);

    auto& steam = engine.GetModule<SteamModule>();
    steam.InitSteamStats(_stats);
    steam.InitSteamAchievements(_achievements);
    steam.RequestCurrentStats();

    // Audio Setup
    auto& audio = engine.GetModule<AudioModule>();

    audio.LoadBank("content/music/Master.strings.bank");
    audio.LoadBank("content/music/Master.bank");

    audio.RegisterChannelBus("bus:/");
    audio.RegisterChannelBus("bus:/SFX");
    audio.RegisterChannelBus("bus:/BGM");

    auto& ECS = engine.GetModule<ECSModule>();
    ECS.AddSystem<LifetimeSystem>();

    GraphicsContext& graphicsContext = *engine.GetModule<RendererModule>().GetGraphicsContext();
    _bindingsVisualizationCache = std::make_unique<InputBindingsVisualizationCache>(actionManager, graphicsContext);

    auto& viewport = engine.GetModule<UIModule>().GetViewport();
    const glm::uvec2 viewportSize = viewport.GetExtend();

    auto font = LoadFromFile("assets/fonts/BLOODROSE.ttf", 100, graphicsContext);
    font->metrics.charSpacing = 0;

    std::string gameVersionText {};
    if (auto versionFile = fileIO::OpenReadStream("version.txt"))
    {
        gameVersionText = fileIO::DumpStreamIntoString(versionFile.value());
        viewport.AddElement(GameVersionVisualization::Create(graphicsContext, viewportSize, font, gameVersionText));
    }
    _gameVersionVisual = viewport.AddElement(GameVersionVisualization::Create(graphicsContext, viewportSize, font, gameVersionText));

    _mainMenu = viewport.AddElement(MainMenu::Create(graphicsContext, viewportSize, font));
    _hud = viewport.AddElement(HUD::Create(graphicsContext, viewportSize, font));
    _loadingScreen = viewport.AddElement(LoadingScreen::Create(graphicsContext, *_bindingsVisualizationCache, viewportSize, font));
    _pauseMenu = viewport.AddElement(PauseMenu::Create(graphicsContext, viewportSize, font));
    _gameOver = viewport.AddElement(GameOverMenu::Create(graphicsContext, viewportSize, font));
    _controlsMenu = viewport.AddElement(ControlsMenu::Create(viewportSize, graphicsContext, *_bindingsVisualizationCache, actionManager, font));
    _creditsMenu = viewport.AddElement(CreditsMenu::Create(engine, graphicsContext, viewportSize, font));

    gameSettings = GameSettings::FromFile(GAME_SETTINGS_FILE);

    ApplySettings(engine);
    _settingsMenu = viewport.AddElement(SettingsMenu::Create(engine, graphicsContext, viewportSize, font));

    // Set all UI menus invisible

    _gameVersionVisual.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _mainMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _hud.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _loadingScreen.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _pauseMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _gameOver.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _controlsMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _settingsMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _creditsMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;

    _framerateCounter = viewport.AddElement(FrameCounter::Create(viewportSize, font));

    // Useful callbacks

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

    auto openSettingsMenu = [this, &engine]()
    {
        this->PushUIMenu(this->_settingsMenu);
        this->PushPreviousFocusedElement(_mainMenu.lock()->settingsButton);
        engine.GetModule<UIModule>().uiInputContext.focusedUIElement = _settingsMenu.lock()->sensitivitySlider;
    };

    auto openSettingsPause = [this, &engine]()
    {
        this->PushUIMenu(this->_settingsMenu);
        this->PushPreviousFocusedElement(_pauseMenu.lock()->settingsButton);
        engine.GetModule<UIModule>().uiInputContext.focusedUIElement = _settingsMenu.lock()->sensitivitySlider;
    };

    _mainMenu.lock()->settingsButton.lock()->OnPress(Callback { openSettingsMenu });
    _pauseMenu.lock()->settingsButton.lock()->OnPress(Callback { openSettingsPause });

    auto openControlsMenu = [this, &engine]()
    {
        this->_controlsMenu.lock()->UpdateBindings();
        this->PushUIMenu(this->_controlsMenu);
        this->PushPreviousFocusedElement(_mainMenu.lock()->controlsButton);
        engine.GetModule<UIModule>().uiInputContext.focusedUIElement = this->_controlsMenu.lock()->backButton;
    };

    auto openControlsPause = [this, &engine]()
    {
        this->_controlsMenu.lock()->UpdateBindings();
        this->PushUIMenu(this->_controlsMenu);
        this->PushPreviousFocusedElement(_pauseMenu.lock()->controlsButton);
        engine.GetModule<UIModule>().uiInputContext.focusedUIElement = this->_controlsMenu.lock()->backButton;
    };

    auto openCredits = [this, &engine]()
    {
        this->PushUIMenu(this->_creditsMenu);
        this->PushPreviousFocusedElement(_mainMenu.lock()->creditsButton);
        engine.GetModule<UIModule>().uiInputContext.focusedUIElement = this->_creditsMenu.lock()->backButton;
    };

    _mainMenu.lock()->creditsButton.lock()->OnPress(Callback { openCredits });

    _mainMenu.lock()->controlsButton.lock()->OnPress(Callback { openControlsMenu });
    _pauseMenu.lock()->controlsButton.lock()->OnPress(Callback { openControlsPause });

    auto closeControls = [&engine]()
    {
        auto& gameModule = engine.GetModule<GameModule>();
        engine.GetModule<UIModule>().uiInputContext.focusedUIElement = gameModule.PopPreviousFocusedElement();
        gameModule.PopUIMenu();
    };

    _controlsMenu.lock()->backButton.lock()->OnPress(Callback { closeControls });

    auto& particleModule = engine.GetModule<ParticleModule>();
    particleModule.LoadEmitterPresets();

    return ModuleTickOrder::eTick;
}

void GameModule::ApplySettings(Engine& engine)
{
    auto curve = [](float normalized_val)
    {
        return normalized_val * normalized_val * 2.0f;
    };

    engine.GetModule<AudioModule>().SetBusChannelVolume("bus:/", curve(gameSettings.masterVolume));
    engine.GetModule<AudioModule>().SetBusChannelVolume("bus:/BGM", curve(gameSettings.musicVolume));
    engine.GetModule<AudioModule>().SetBusChannelVolume("bus:/SFX", curve(gameSettings.sfxVolume));

    if (auto locked = _settingsMenu.lock(); locked != nullptr)
    {
        if (auto lockedFovText = locked->fovText.lock(); lockedFovText != nullptr)
        {
            lockedFovText->SetText(std::to_string(static_cast<int>(50.0f + gameSettings.fov * 100.0f)));
        }
    }

    auto& swapchain = engine.GetModule<RendererModule>().GetRenderer()->GetSwapChain();
    if (swapchain.SetPresentMode(gameSettings.vsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox))
    {
        swapchain.Resize(swapchain.GetImageSize());
    }

    // Frame counter

    if (auto counter = _framerateCounter.lock())
    {

        if (gameSettings.framerateCounter)
        {
            counter->visibility = UIElement::VisibilityState::eUpdatedAndVisible;
            auto dt = engine.GetModule<TimeModule>().GetRealDeltatime();

            if (dt.count() != 0.0f)
                counter->SetVal(1000.0f / dt.count());
        }
        else
        {
            counter->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
        }
    }
}

void GameModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    gameSettings.SaveToFile(GAME_SETTINGS_FILE);
}

std::optional<std::shared_ptr<MainMenu>> GameModule::GetMainMenu()
{
    if (auto lock = _mainMenu.lock())
    {
        return lock;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<PauseMenu>> GameModule::GetPauseMenu()
{
    if (auto lock = _pauseMenu.lock())

    {
        return lock;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<HUD>> GameModule::GetHUD()
{
    if (auto lock = _hud.lock())
    {
        return lock;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<GameOverMenu>> GameModule::GetGameOver()
{
    if (auto lock = _gameOver.lock())
    {
        return lock;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<LoadingScreen>> GameModule::GetLoadingScreen()
{
    if (auto lock = _loadingScreen.lock())
    {
        return lock;
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<GameVersionVisualization>> GameModule::GetGameVersionVisual()
{
    if (auto lock = _gameVersionVisual.lock())
    {
        return lock;
    }
    return std::nullopt;
}

void GameModule::SetUIMenu(std::weak_ptr<Canvas> menu)
{
    _menuStack = {};
    _menuStack.push(menu);
}
void GameModule::PushUIMenu(std::weak_ptr<Canvas> menu)
{
    _menuStack.push(menu);
}

void GameModule::PopUIMenu()
{
    if (!_menuStack.empty())
    {
        _menuStack.pop();
    }
}

std::weak_ptr<UIElement> GameModule::PopPreviousFocusedElement()
{
    auto elem = _focusedElementStack.top();
    _focusedElementStack.pop();
    return elem;
}

void GameModule::PushPreviousFocusedElement(std::weak_ptr<UIElement> element)
{
    _focusedElementStack.push(element);
}

void GameModule::SetNextScene(const std::string& scriptFile)
{
    _nextSceneToExecute = scriptFile;
}

void GameModule::TransitionScene(Engine& engine)
{
    // Cleanup for some modules
    engine.GetModule<ECSModule>().GetRegistry().clear();
    engine.GetModule<AudioModule>().Reset();
    engine.GetModule<RendererModule>().GetRenderer()->GetGPUScene().ResetDecals();
    engine.GetModule<RendererModule>().GetRenderer()->GetParticlePipeline().ResetParticles();
    flashColor = {};

    engine.GetModule<ScriptingModule>().SetMainScript(engine, _nextSceneToExecute);

    engine.GetModule<TimeModule>().ResetTimer();
}

void GameModule::Tick(Engine& engine)
{
    ApplySettings(engine);

    if (!_nextSceneToExecute.empty())
    {
        TransitionScene(engine);
    }

    _nextSceneToExecute.clear();

    auto& ECS = engine.GetModule<ECSModule>();

    auto& applicationModule = engine.GetModule<ApplicationModule>();
    auto& physicsModule = engine.GetModule<PhysicsModule>();
    auto& pathfindingModule = engine.GetModule<PathfindingModule>();

    // Slow down application when minimized.
    if (applicationModule.isMinimized())
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(16ms);
        return;
    }

    // Pass the flash color to rendering
    auto& renderer = engine.GetModule<RendererModule>();
    renderer.GetRenderer()->GetTonemappingPipeline().SetFlashColor(flashColor);

    // Handle UI stack

    _mainMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _hud.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _loadingScreen.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _pauseMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _gameOver.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _settingsMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _controlsMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;
    _creditsMenu.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisible;

    if (!_menuStack.empty())
    {
        _menuStack.top().lock()->visibility = UIElement::VisibilityState::eUpdatedAndVisible;
    }

#if !DISTRIBUTION
    auto& inputDeviceManager = applicationModule.GetInputDeviceManager();
    if (inputDeviceManager.IsKeyPressed(KeyboardCode::eH))
    {
        applicationModule.SetMouseHidden(!applicationModule.GetMouseHidden());
    }
#endif

    {
        // TODO!!! This can be directly handled by the debug renderer/physics
        ZoneNamedN(updateCamera, "Update Physics camera", true);

        auto cameraView = ECS.GetRegistry().view<CameraComponent, TransformComponent>();
        for (const auto& [entity, cameraComponent, transformComponent] : cameraView.each())
        {
            auto windowSize = applicationModule.DisplaySize();
            cameraComponent.aspectRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);

            glm::vec3 position = TransformHelpers::GetWorldPosition(ECS.GetRegistry(), entity);
            physicsModule.SetDebugCameraPosition(position);
        }
    }

    if (pathfindingModule.GetDebugDrawState())
    {
        const std::vector<glm::vec3>& pathfindingLines = pathfindingModule.GetDebugLines();
        engine.GetModule<RendererModule>().GetRenderer()->GetDebugPipeline().AddLines(pathfindingLines);
    }
}
