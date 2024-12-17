#include "ui_module.hpp"
#include "canvas.hpp"
#include <application_module.hpp>
#include <renderer.hpp>
#include <renderer_module.hpp>

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    _viewport = std::make_unique<Viewport>(extend);
    _graphicsContext = engine.GetModule<RendererModule>().GetGraphicsContext();
    return ModuleTickOrder::ePostTick;
}
void UIModule::Tick(Engine& engine)
{
    _viewport->Update(engine.GetModule<ApplicationModule>().GetActionManager());
}

void UIModule::CreateMainMenu(std::function<void()> onPlayButtonClick, std::function<void()> onExitButtonClick)
{
    GetViewport().AddElement<Canvas>(CreateMainMenuCanvas(_viewport->GetExtend(), _graphicsContext, onPlayButtonClick, onExitButtonClick));
}
