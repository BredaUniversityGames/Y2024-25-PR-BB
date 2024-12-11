#include "inspector_module.hpp"
#include "editor.hpp"
#include "imgui_backend.hpp"
#include "implot/implot.h"
#include "menus/performance_tracker.hpp"
#include "profile_macros.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"

#include <scripting_module.hpp>

InspectorModule::InspectorModule() = default;

InspectorModule::~InspectorModule()
{
}

ModuleTickOrder InspectorModule::Init(Engine& engine)
{
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    auto& ecs = engine.GetModule<ECSModule>();
    auto& renderer = engine.GetModule<RendererModule>();
    auto& window = engine.GetModule<ApplicationModule>();

    _imguiBackend = std::make_shared<ImGuiBackend>(
        renderer.GetRenderer()->GetContext(),
        window, renderer.GetRenderer()->GetSwapChain(),
        renderer.GetRenderer()->GetGBuffers());

    _editor = std::make_unique<Editor>(ecs, renderer.GetRenderer(), _imguiBackend);
    _performanceTracker = std::make_unique<PerformanceTracker>();

    return ModuleTickOrder::ePostTick;
}

void InspectorModule::Shutdown(Engine& engine)
{
    if (auto* ptr = engine.GetModuleSafe<RendererModule>())
    {
        ptr->GetRenderer()->FlushCommands();
    }
    _editor.reset();
}

void InspectorModule::Tick(Engine& engine)
{
    _imguiBackend->NewFrame();
    ImGui::NewFrame();

    _performanceTracker->Update();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::Button("Reload Game"))
        {
            bblog::info("Hot reloaded environment!");
            engine.GetModule<ScriptingModule>().HotReload(engine);
        }
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load Scene"))
            {
                // Todo: Load saved scene.
            }
            if (ImGui::MenuItem("Save Scene"))
            {
                // Serialization::SerialiseToJSON("assets/maps/scene.json", _ecs);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    _editor->Draw(
        *_performanceTracker,
        engine.GetModule<RendererModule>().GetRenderer()->GetBloomSettings());

    {
        ZoneNamedN(zz, "ImGui Render", true);
        ImGui::Render();
    }
}