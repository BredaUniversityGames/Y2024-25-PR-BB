#include "application_module.hpp"
#include "log.hpp"
#include "input_manager.hpp"
#include "engine.hpp"

#define SDL_DISABLE_ANALYZE_MACROS
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <backends/imgui_impl_sdl3.h>

ModuleTickOrder ApplicationModule::Init(Engine& engine)
{
    ModuleTickOrder priority = ModuleTickOrder::eLast;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        bblog::error("Failed initializing SDL: {0}", SDL_GetError());
        engine.SetExit(-1);
        return priority;
    }

    int32_t displayCount;
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

    uint32_t sdlExtensionsCount = 0;
    _vulkanInitInfo.extensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionsCount);
    _vulkanInitInfo.extensionCount = sdlExtensionsCount;

    _vulkanInitInfo.width = dm->w;
    _vulkanInitInfo.height = dm->h;
    _vulkanInitInfo.retrieveSurface = [this](vk::Instance instance)
    {
        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface))
        {
            bblog::error("Failed creating SDL vk::Surface: {}", SDL_GetError());
        }
        return vk::SurfaceKHR(surface);
    };

    _inputManager = std::make_unique<InputManager>();
    SetMouseHidden(_mouseHidden);

    return priority;
}
void ApplicationModule::Shutdown(Engine& engine)
{
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void ApplicationModule::Tick(Engine& engine)
{
    _inputManager->Update();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        _inputManager->UpdateEvent(event);
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EventType::SDL_EVENT_QUIT)
        {
            engine.SetExit(0);
            break;
        }
    }
}

ApplicationModule::ApplicationModule() = default;
ApplicationModule::~ApplicationModule() = default;

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
    int32_t w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return glm::uvec2 { w, h };
}
bool ApplicationModule::isMinimized() const
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(_window);
    return flags & SDL_WINDOW_MINIMIZED;
}