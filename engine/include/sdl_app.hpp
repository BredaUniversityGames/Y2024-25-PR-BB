#pragma once

#include "application.hpp"

class SDLApp : public Application
{
public:
    SDLApp(const CreateParameters& parameters);

    ~SDLApp() override;

    NON_COPYABLE(SDLApp);
    NON_MOVABLE(SDLApp);

    InitInfo GetInitInfo() override;

    glm::uvec2 DisplaySize() override;

    bool IsMinimized() override;

    void InitImGui() override;

    void NewImGuiFrame() override;

    void ShutdownImGui() override;

    void SetMouseHidden(bool state) override;
    void ProcessWindowEvents() override;
    bool GetMouseHidden() override { return _mouseHidden;}

    const InputManager& GetInputManager() const override;

private:
    SDL_Window* _window;

    InitInfo _initInfo;

    class InputManager _inputManager;

    bool _mouseHidden = false;
};