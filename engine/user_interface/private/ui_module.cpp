#include "ui_module.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include <application_module.hpp>
#include <canvas.hpp>
#include <fonts.hpp>
#include <renderer.hpp>
#include <renderer_module.hpp>
#include <ui_button.hpp>
#include <ui_text.hpp>

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    _viewport = std::make_unique<Viewport>(extend);
    _graphicsContext = engine.GetModule<RendererModule>().GetGraphicsContext();
    return ModuleTickOrder::ePostTick;
}
void UIModule::Tick(Engine& engine)
{
    _viewport->Update(engine.GetModule<ApplicationModule>().GetInputManager());
}

void UIModule::CreateMainMenu(std::function<void()> onPlayButtonClick, std::function<void()> onExitButtonClick)
{
    GetViewport().AddElement<Canvas>(CreateMainMenuCanvas(_viewport->GetExtend(), _graphicsContext, onPlayButtonClick, onExitButtonClick));
}
