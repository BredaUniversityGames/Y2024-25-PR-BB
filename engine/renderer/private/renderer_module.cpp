#include "renderer_module.hpp"

#include "animation_system.hpp"
#include "application_module.hpp"
#include "ecs_module.hpp"
#include "engine.hpp"
#include "graphics_context.hpp"
#include "renderer.hpp"
#include "ui_module.hpp"
#include "vulkan_context.hpp"

#include <imgui.h>
#include <implot.h>
#include <memory>
#include <time_module.hpp>
#include <tracy/Tracy.hpp>

RendererModule::RendererModule()
{
}

ModuleTickOrder RendererModule::Init(Engine& engine)
{
    auto& ecs = engine.GetModule<ECSModule>();
    _context = std::make_shared<GraphicsContext>(engine.GetModule<ApplicationModule>().GetVulkanInfo());

    _renderer = std::make_shared<Renderer>(engine.GetModule<ApplicationModule>(), engine.GetModule<UIModule>().GetViewport(), _context, ecs);

    ecs.AddSystem<AnimationSystem>(*this);

    return ModuleTickOrder::eRender;
}

void RendererModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _context->VulkanContext()->Device().waitIdle();
    _renderer.reset();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    _context.reset();
}

void RendererModule::Tick(MAYBE_UNUSED Engine& engine)
{
    auto dt = engine.GetModule<TimeModule>().GetDeltatime();
    _renderer->Render(dt.count());
}

std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>> RendererModule::FrontLoadModels(const std::vector<std::string>& modelPaths)
{
    auto result = _renderer->FrontLoadModels(modelPaths);

    _renderer->UpdateBindless();

    return result;
}
