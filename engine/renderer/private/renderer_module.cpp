#include "renderer_module.hpp"

#include "application_module.hpp"
#include "engine.hpp"
#include "graphics_context.hpp"
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
    _renderer = std::make_unique<Renderer>(engine.GetModule<ApplicationModule>(), ecs);
    _particleInterface = std::make_unique<ParticleInterface>(ecs);

    return ModuleTickOrder::eRender;
}

void RendererModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    _renderer->GetContext()->VulkanContext()->Device().waitIdle();

    _particleInterface.reset();
    _renderer.reset();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

void RendererModule::Tick(MAYBE_UNUSED Engine& engine)
{
}

void RendererModule::SetScene(std::shared_ptr<const SceneDescription> scene)
{
    _renderer->_scene = scene;
}

std::vector<Model> RendererModule::FrontLoadModels(const std::vector<std::string>& modelPaths)
{
    auto result = _renderer->FrontLoadModels(modelPaths);

    _renderer->UpdateBindless();

    return result;
}
