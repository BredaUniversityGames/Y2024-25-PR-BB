#include "application_module.hpp"
#include "engine.hpp"
#include "input/sdl/sdl_action_manager.hpp"
#include "input/sdl/sdl_input_device_manager.hpp"
#include "input/steam/steam_action_manager.hpp"
#include "input/steam/steam_input_device_manager.hpp"
#include "log.hpp"
#include "steam_module.hpp"

// SDL throws some weird errors when parsed with clang-analyzer (used in clang-tidy checks)
// This definition fixes the issues and does not change the final build output
#define SDL_DISABLE_ANALYZE_MACROS

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_sdl3.h>
#include <stb_image.h>

ModuleTickOrder ApplicationModule::Init(Engine& engine)
{
    ModuleTickOrder priority = ModuleTickOrder::eLast;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        bblog::error("Failed initializing SDL: {0}", SDL_GetError());
        engine.SetExit(-1);
        return priority;
    }

    int32_t displayCount {};
    SDL_DisplayID* displayIds = SDL_GetDisplays(&displayCount);
    const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(*displayIds);

    if (dm == nullptr)
    {
        bblog::error("Failed retrieving DisplayMode: {0}", SDL_GetError());
        engine.SetExit(-1);
        return priority;
    }

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
    if (_isFullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    _window = SDL_CreateWindow(_windowName.data(), dm->w, dm->h, flags);

    if (_window == nullptr)
    {
        bblog::error("Failed creating SDL window: {}", SDL_GetError());
        engine.SetExit(-1);
        SDL_Quit();
        return priority;
    }

    int32_t width, height, nrChannels;
    stbi_uc* pixels = stbi_load("assets/textures/icon.png", &width, &height, &nrChannels, 3);
    if (pixels)
    {
        SDL_Surface* icon = SDL_CreateSurfaceFrom(
            width,
            height,
            SDL_PIXELFORMAT_RGB24,
            pixels, width * 3);
        SDL_SetWindowIcon(_window, icon);

        SDL_DestroySurface(icon);
    }
    else
    {
        bblog::warn("Unable to load window icon!");
    }

    uint32_t sdlExtensionsCount = 0;
    _vulkanInitInfo.extensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionsCount);
    _vulkanInitInfo.extensionCount = sdlExtensionsCount;

    _vulkanInitInfo.width = dm->w;
    _vulkanInitInfo.height = dm->h;
    _vulkanInitInfo.retrieveSurface = [this](vk::Instance instance)
    {
        VkSurfaceKHR surface {};
        if (!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface))
        {
            bblog::error("Failed creating SDL vk::Surface: {}", SDL_GetError());
        }
        return vk::SurfaceKHR(surface);
    };

    const SteamModule& steam = engine.GetModule<SteamModule>();
    if (steam.InputAvailable())
    {
        bblog::info("Steam Input available, creating SteamActionManager. Controller input settings will be used from Steam");
        _inputDeviceManager = std::make_unique<SteamInputDeviceManager>();
        const SteamInputDeviceManager& inputManager = dynamic_cast<SteamInputDeviceManager&>(*_inputDeviceManager);
        _actionManager = std::make_unique<SteamActionManager>(inputManager);
    }
    else
    {
        bblog::info("Steam Input not available, creating default ActionManager. Controller input settings will be used from program");
        _inputDeviceManager = std::make_unique<SDLInputDeviceManager>();
        const SDLInputDeviceManager& inputManager = dynamic_cast<SDLInputDeviceManager&>(*_inputDeviceManager);
        _actionManager = std::make_unique<SDLActionManager>(inputManager);
    }

    SetMouseHidden(_mouseHidden);

    return priority;
}

void ApplicationModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void ApplicationModule::Tick(Engine& engine)
{
    _inputDeviceManager->Update();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        _inputDeviceManager->UpdateEvent(event);

        if (_mouseHidden == false)
        {
            _inputDeviceManager->SetMousePositionToAbsoluteMousePosition();
        }

        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EventType::SDL_EVENT_QUIT)
        {
            engine.SetExit(0);
            break;
        }
    }

    _actionManager->Update();
}

ApplicationModule::ApplicationModule() = default;

void ApplicationModule::SetMouseHidden(bool val)
{
    _mouseHidden = val;

    // SDL_SetWindowMouseGrab(_window, _mouseHidden);
    SDL_SetWindowRelativeMouseMode(_window, _mouseHidden);

    if (_mouseHidden)
        SDL_HideCursor();
    else
        SDL_ShowCursor();
}

glm::uvec2 ApplicationModule::DisplaySize() const
{
    int32_t w {}, h {};
    SDL_GetWindowSize(_window, &w, &h);
    return glm::uvec2 { w, h };
}
bool ApplicationModule::isMinimized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(_window);
    return flags & SDL_WINDOW_MINIMIZED;
}