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

namespace InspectInternal
{
void DumpVMAStats(Engine& engine);
void DrawRenderStats(Engine& engine);
void DrawSSAOSettings(Engine& engine);
void DrawFXAASettings(Engine& engine);
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

    _editor = std::make_unique<Editor>(ecs);
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
            if (ImGui::MenuItem("Dump VMA stats to json"))
            {
                InspectInternal::DumpVMAStats(engine);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Renderer"))
        {
            ImGui::MenuItem("Bloom Settings", nullptr, &_openWindows["Bloom Settings"]);
            ImGui::MenuItem("Shadow map visualisation", nullptr, &_openWindows["shadow map"]);
            ImGui::MenuItem("SSAO Settings", nullptr, &_openWindows["SSAO"]);
            ImGui::MenuItem("FXAA Settings", nullptr, &_openWindows["FXAA"]);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Info##Bar"))
        {
            ImGui::MenuItem("Performance Tracker", nullptr, &_openWindows["Performance"]);
            ImGui::MenuItem("Draw Stats", nullptr, &_openWindows["RenderStats"]);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("ECS"))
        {
            ImGui::MenuItem("Hierarchy", nullptr, &_openWindows["Hierarchy"]);

            for (const auto& system : engine.GetModule<ECSModule>().GetSystems())
            {
                ImGui::MenuItem(system->GetName().data(), nullptr, &_openWindows[system->GetName().data()]);
            }

            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (_openWindows["Hierarchy"])
    {
        _editor->DrawHierarchy();
    }

    if (_openWindows["RenderStats"])
    {
        _performanceTracker->Render();
    }

    if (_openWindows["Performance"])
    {
        InspectInternal::DrawRenderStats(engine);
    }

    if (_openWindows["Bloom Settings"])
    {
        engine.GetModule<RendererModule>().GetRenderer()->GetBloomSettings().Render();
    }

    if (_openWindows["SSAO"])
    {
        InspectInternal::DrawSSAOSettings(engine);
    }

    if (_openWindows["FXAA"])
    {
        InspectInternal::DrawFXAASettings(engine);
    }

    {
        ZoneNamedN(systemInspect, "System inspect", true);
        for (const auto& system : engine.GetModule<ECSModule>().GetSystems())
        {
            if (_openWindows[system->GetName().data()])
            {
                system->Inspect();
            }
        }
    }

    _editor->Draw();

    {
        ZoneNamedN(zz, "ImGui Render", true);
        ImGui::Render();
    }
}

namespace InspectInternal
{
void DrawRenderStats(Engine& engine)
{
    const auto stats = engine.GetModule<RendererModule>().GetRenderer()->GetContext()->GetDrawStats();

    ImGui::Begin("Renderer Stats");

    ImGui::LabelText("Draw calls", "%i", stats.DrawCalls());
    ImGui::LabelText("Triangles", "%i", stats.IndexCount() / 3);
    ImGui::LabelText("Indirect draw commands", "%i", stats.IndirectDrawCommands());
    ImGui::LabelText("Particles after simulation", "%i", stats.GetParticleCount());
    ImGui::LabelText("Emitters", "%i", stats.GetEmitterCount());

    ImGui::End();
}

void DumpVMAStats(Engine& engine)
{
    char* statsJson {};
    vmaBuildStatsString(engine.GetModule<RendererModule>().GetRenderer()->GetContext()->VulkanContext()->MemoryAllocator(), &statsJson, true);

    const char* outputFilePath = "vma_stats.json";
    std::ofstream file { outputFilePath };
    if (file.is_open())
    {
        file << statsJson;

        file.close();
    }
    else
    {
        bblog::error("Failed writing VMA stats to file!");
    }
    vmaFreeStatsString(engine.GetModule<RendererModule>().GetRenderer()->GetContext()->VulkanContext()->MemoryAllocator(), statsJson);
}

void DrawSSAOSettings(Engine& engine)
{
    auto& ssao = engine.GetModule<RendererModule>().GetRenderer()->GetSSAOPipeline();

    ImGui::Begin("SSAO settings");
    ImGui::DragFloat("AO strength", &ssao.GetAOStrength(), 0.1f, 0.0f, 16.0f);
    ImGui::DragFloat("Bias", &ssao.GetAOBias(), 0.001f, 0.0f, 0.1f);
    ImGui::DragFloat("Radius", &ssao.GetAORadius(), 0.05f, 0.0f, 2.0f);
    ImGui::DragFloat("Minimum AO distance", &ssao.GetMinAODistance(), 0.05f, 0.0f, 1.0f);
    ImGui::DragFloat("Maximum AO distance", &ssao.GetMaxAODistance(), 0.05f, 0.0f, 1.0f);
    ImGui::End();
}

void DrawFXAASettings(Engine& engine)
{
    auto& fxaa = engine.GetModule<RendererModule>().GetRenderer()->GetFXAAPipeline();

    ImGui::Begin("FXAA settings");
    ImGui::Checkbox("Enable FXAA", &fxaa.GetEnableFXAA());
    ImGui::DragFloat("Edge treshold min", &fxaa.GetEdgeTreshholdMin(), 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Edge treshold max", &fxaa.GetEdgeTreshholdMax(), 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Subpixel quality", &fxaa.GetSubPixelQuality(), 0.01f, 0.0f, 1.0f);
    ImGui::DragInt("Iterations", &fxaa.GetIterations(), 1, 1, 128);
    ImGui::End();
}
}