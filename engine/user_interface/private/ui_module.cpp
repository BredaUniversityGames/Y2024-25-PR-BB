#include "ui_module.hpp"
#include <application_module.hpp>

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    viewport = std::make_unique<Viewport>(extend);
    return ModuleTickOrder::ePostTick;
}
void UIModule::Tick(Engine& engine)
{
    viewport->Update(engine.GetModule<ApplicationModule>().GetInputManager());
}