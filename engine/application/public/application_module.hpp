#pragma once
#include "module_interface.hpp"
#include <functional>
#include <glm/vec2.hpp>
#include <memory>

#include <vulkan/vulkan.hpp>

// Undefining problematic X11 defines

#undef Bool
#undef None
#undef Convex

class InputManager;
struct SDL_Window;

class ApplicationModule : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

public:
    ApplicationModule();

    // TODO: Maybe move to a GPU/Vulkan Context module
    struct VulkanInitInfo
    {
        uint32_t extensionCount { 0 };
        const char* const* extensions { nullptr };
        uint32_t width {}, height {};

        std::function<vk::SurfaceKHR(vk::Instance)> retrieveSurface;
    };

    [[nodiscard]] SDL_Window* GetWindowHandle() const { return _window; }
    [[nodiscard]] VulkanInitInfo GetVulkanInfo() const { return _vulkanInitInfo; }
    [[nodiscard]] InputManager& GetInputManager() const { return *_inputManager; }

    [[nodiscard]] bool GetMouseHidden() const { return _mouseHidden; }
    void SetMouseHidden(bool val);

    [[nodiscard]] glm::uvec2 DisplaySize() const;
    [[nodiscard]] bool isMinimized() const;

private:
    std::unique_ptr<InputManager> _inputManager {};
    SDL_Window* _window = nullptr;
    VulkanInitInfo _vulkanInitInfo;

    std::string _windowName = "BB-Prototype";
    bool _isFullscreen = true;
    bool _mouseHidden = true;
    uint32_t _width {}, _height {};
};