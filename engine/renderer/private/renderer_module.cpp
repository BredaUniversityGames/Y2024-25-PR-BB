#include "renderer_module.hpp"

#include "animation_system.hpp"
#include "application_module.hpp"
#include "ecs_module.hpp"
#include "engine.hpp"
#include "graphics_context.hpp"
#include "imgui_backend.hpp"
#include "particle_module.hpp"
#include "renderer.hpp"
#include "ui_module.hpp"
#include "vulkan_context.hpp"

#include <imgui.h>
#include <implot.h>
#include <memory>

RendererModule::RendererModule()
{
}

ModuleTickOrder RendererModule::Init(Engine& engine)
{
    auto& ecs = engine.GetModule<ECSModule>();
    _context = std::make_shared<GraphicsContext>(engine.GetModule<ApplicationModule>().GetVulkanInfo());

    _renderer = std::make_shared<Renderer>(engine.GetModule<ApplicationModule>(), engine.GetModule<UIModule>().GetViewport(), _context, ecs);

    _imguiBackend = std::make_shared<ImGuiBackend>(_renderer->GetContext(), engine.GetModule<ApplicationModule>(), _renderer->GetSwapChain(), _renderer->GetGBuffers());

    ecs.AddSystem<AnimationSystem>(*this);

    return ModuleTickOrder::eRender;
}

void RendererModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _context->VulkanContext()->Device().waitIdle();
    _imguiBackend.reset();

    _renderer.reset();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    _context.reset();
}

void RendererModule::Tick(MAYBE_UNUSED Engine& engine)
{
}

std::vector<std::pair<CPUModel, ResourceHandle<GPUModel>>> RendererModule::FrontLoadModels(const std::vector<std::string>& modelPaths)
{
    auto result = _renderer->FrontLoadModels(modelPaths);

    _renderer->UpdateBindless();

    return result;
}
