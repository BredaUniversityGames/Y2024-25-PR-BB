#include "ui_module.hpp"
#include "application_module.hpp"
#include "canvas.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "time_module.hpp"

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    _viewport = std::make_unique<Viewport>(extend);
    Viewport::SetScaleMultiplier(Viewport::CalculateScaleMultiplierFromVerticalResolution(extend.y));
    _graphicsContext = engine.GetModule<RendererModule>().GetGraphicsContext();
    return ModuleTickOrder::ePostTick;
}

void UIModule::Tick(Engine& engine)
{
    uiInputContext._gamepadHasFocus = engine.GetModule<ApplicationModule>().GetInputDeviceManager().IsGamepadAvailable();
    uiInputContext.deltatime = engine.GetModule<TimeModule>().GetRealDeltatime();

    InputManagers inputManagers {
        .inputDeviceManager = engine.GetModule<ApplicationModule>().GetInputDeviceManager(),
        .actionManager = engine.GetModule<ApplicationModule>().GetActionManager()
    };

    _viewport->Update(inputManagers, uiInputContext);
    uiInputContext._hasInputBeenConsumed = false;
}
