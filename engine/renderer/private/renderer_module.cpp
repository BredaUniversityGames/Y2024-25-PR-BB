#include "renderer_module.hpp"

#include "application_module.hpp"
#include "engine.hpp"
#include "graphics_context.hpp"
#include "imgui_backend.hpp"
#include "old_engine.hpp"
#include "renderer.hpp"
#include "vulkan_context.hpp"

#include <imgui.h>
#include <implot.h>
#include <memory>

RendererModule::RendererModule()
{
}

ModuleTickOrder RendererModule::Init(Engine& engine)
{
    auto ecs = engine.GetModule<OldEngine>().GetECS();
    _context = std::make_shared<GraphicsContext>(engine.GetModule<ApplicationModule>().GetVulkanInfo());
    _renderer = std::make_shared<Renderer>(engine.GetModule<ApplicationModule>(), _context, ecs);
    _particleInterface = std::make_unique<ParticleInterface>(ecs);
    _imguiBackend = std::make_shared<ImGuiBackend>(_renderer->GetContext(), engine.GetModule<ApplicationModule>(), _renderer->GetSwapChain(), _renderer->GetGBuffers());

    return ModuleTickOrder::eRender;
}

void RendererModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _context->VulkanContext()->Device().waitIdle();

    _particleInterface.reset();
    _imguiBackend.reset();

    _renderer.reset();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    _context.reset();
}

void RendererModule::Tick(MAYBE_UNUSED Engine& engine)
{
}

void RendererModule::SetScene(std::shared_ptr<const SceneDescription> scene)
{
    _renderer->_scene = scene;
}

std::vector<CPUModelData> RendererModule::FrontLoadModels(const std::vector<std::string>& modelPaths)
{

    bblog::error("NOT IMPLEMENTED!!, RendererModule::FrontLoadModels");

    auto result = _renderer->FrontLoadModels(modelPaths);

    _renderer->UpdateBindless();

    return result;
}
