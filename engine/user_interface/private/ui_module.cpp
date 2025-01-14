#include "ui_module.hpp"
#include "application_module.hpp"
#include "canvas.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "ui_menus.hpp"

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    _viewport = std::make_unique<Viewport>(extend);
    _graphicsContext = engine.GetModule<RendererModule>().GetGraphicsContext();
    return ModuleTickOrder::ePostTick;
}

void UIModule::Tick(Engine& engine)
{
    _uiInputContext._gamepadHasFocus = engine.GetModule<ApplicationModule>().GetInputDeviceManager().IsGamepadAvailable();

    InputManagers inputManagers {
        .inputDeviceManager = engine.GetModule<ApplicationModule>().GetInputDeviceManager(),
        .actionManager = engine.GetModule<ApplicationModule>().GetActionManager()
    };

    _viewport->Update(inputManagers, _uiInputContext);
    _uiInputContext._hasInputBeenConsumed = false;
}

void UIModule::CreateNavigationTest()
{
    GetViewport().AddElement<Canvas>(CreateMainMenu(_uiInputContext, _viewport->GetExtend(), _graphicsContext));
}
