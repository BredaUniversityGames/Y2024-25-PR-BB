#include "ui_module.hpp"
#include <application_module.hpp>
#include <tracy/Tracy.hpp>

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    _viewport = std::make_unique<Viewport>(extend);
    return ModuleTickOrder::ePostTick;
}
void UIModule::Tick(Engine& engine)
{
    ZoneScoped;
    _viewport->Update(engine.GetModule<ApplicationModule>().GetInputDeviceManager());
}