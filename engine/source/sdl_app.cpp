#include "sdl_app.hpp"
#include "SDL3/SDL.h"
#include "SDL3/SDL_vulkan.h"
#include "imgui/backends/imgui_impl_sdl3.h"

SDLApp::SDLApp(const CreateParameters& parameters)
    : Application(parameters)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        spdlog::error("Failed initializing SDL: {0}", SDL_GetError());
        return;
    }

    int32_t displayCount;
    SDL_DisplayID* displayIds = SDL_GetDisplays(&displayCount);
    const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(*displayIds);
    if (dm == nullptr)
    {
        spdlog::error("Failed retrieving DisplayMode: {0}", SDL_GetError());
        return;
    }

    SDL_WindowFlags flags = SDL_WINDOW_VULKAN;
    if (parameters.isFullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    _window = SDL_CreateWindow(parameters.windowTitle.data(), dm->w, dm->h, flags);

    if (_window == nullptr)
    {
        spdlog::error("Failed creating SDL window: {}", SDL_GetError());
        SDL_Quit();
        return;
    }

    uint32_t sdlExtensionsCount = 0;
    _initInfo.extensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionsCount);
    _initInfo.extensionCount = sdlExtensionsCount;

    _initInfo.width = dm->w;
    _initInfo.height = dm->h;
    _initInfo.retrieveSurface = [this](vk::Instance instance)
    {
        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(_window, instance, nullptr, &surface))
        {
            spdlog::error("Failed creating SDL vk::Surface: {}", SDL_GetError());
        }
        return vk::SurfaceKHR(surface);
    };
}

SDLApp::~SDLApp()
{
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

InitInfo SDLApp::GetInitInfo()
{
    return _initInfo;
}

glm::uvec2 SDLApp::DisplaySize()
{
    int32_t w, h;
    SDL_GetWindowSize(_window, &w, &h);
    return glm::uvec2 { w, h };
}

bool SDLApp::IsMinimized()
{
    SDL_WindowFlags flags = SDL_GetWindowFlags(_window);
    return flags & SDL_WINDOW_MINIMIZED;
}

void SDLApp::Run(std::function<bool()> updateLoop)
{
    bool running = true;
    while (running)
    {
        _inputManager.Update();

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            _inputManager.UpdateEvent(event);
            if (event.type == SDL_EventType::SDL_EVENT_QUIT)
                running = false;
        }

        if (updateLoop())
            running = false;
        FrameMark;
    }
}

void SDLApp::InitImGui()
{
    ImGui_ImplSDL3_InitForVulkan(_window);
}

void SDLApp::NewImGuiFrame()
{
    ZoneScoped;
    ImGui_ImplSDL3_NewFrame();
}

void SDLApp::ShutdownImGui()
{
    ImGui_ImplSDL3_Shutdown();
}

const class InputManager& SDLApp::GetInputManager() const
{
    return _inputManager;
}

void SDLApp::SetMouseHidden(bool state)
{
    _mouseHidden = state;

    // SDL_SetWindowMouseGrab(_window, _mouseHidden);
    SDL_SetWindowRelativeMouseMode(_window, _mouseHidden);

    if (_mouseHidden)
        SDL_HideCursor();
    else
        SDL_ShowCursor();
}
