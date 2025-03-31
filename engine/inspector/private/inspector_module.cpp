#include "inspector_module.hpp"

#include "editor.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "imgui_backend.hpp"
#include "implot.h"
#include "magic_enum.hpp"
#include "menus/particle_editor.hpp"
#include "menus/performance_tracker.hpp"
#include "passes/shadow_pass.hpp"
#include "renderer.hpp"
#include "renderer_module.hpp"
#include "scripting_module.hpp"
#include "settings.hpp"
#include "tonemapping_functions.hpp"
#include "tracy/Tracy.hpp"
#include "vulkan_context.hpp"

#include "components/static_mesh_component.hpp"
#include "components/wants_shadows_updated.hpp"

InspectorModule::InspectorModule() = default;

InspectorModule::~InspectorModule()
{
}

void DumpVMAStats(Engine& engine);
void DrawRenderStats(Engine& engine);
void DrawBloomSettings(Settings& settings);
void DrawSSAOSettings(Settings& settings);
void DrawFXAASettings(Settings& settings);
void DrawFogSettings(Settings& settings);
void DrawTonemappingSettings(Settings& settings);
void DrawLightingSettings(Settings& settings);
void DrawShadowMapInspect(Engine& engine, ImGuiBackend& imguiBackend);

inline void SetupImGuiStyle();

ModuleTickOrder InspectorModule::Init(Engine& engine)
{
    ImGui::CreateContext();
    ImPlot::CreateContext();

    SetupImGuiStyle();

    ImGuiIO& io
        = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Medium.ttf", 16.0f);

    auto& ecs
        = engine.GetModule<ECSModule>();
    auto& renderer = engine.GetModule<RendererModule>();
    auto& window = engine.GetModule<ApplicationModule>();

    _imguiBackend = std::make_shared<ImGuiBackend>(
        renderer.GetRenderer()->GetContext(),
        window, renderer.GetRenderer()->GetSwapChain(),
        renderer.GetRenderer()->GetGBuffers());

    _editor = std::make_unique<Editor>(ecs);
    _performanceTracker = std::make_unique<PerformanceTracker>();
    _particleEditor = std::make_unique<ParticleEditor>(engine.GetModule<ParticleModule>(), ecs);

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

void InspectorModule::Tick(MAYBE_UNUSED Engine& engine)
{
    _imguiBackend->NewFrame();
    ImGui::NewFrame();

#ifndef DISTRIBUTION
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
                DumpVMAStats(engine);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Renderer"))
        {
            ImGui::MenuItem("Performance Tracker", nullptr, &_openWindows["Performance"]);
            ImGui::MenuItem("Draw Stats", nullptr, &_openWindows["RenderStats"]);
            ImGui::MenuItem("Shadow map visualisation", nullptr, &_openWindows["Shadow Map"]);
            ImGui::MenuItem("Bloom Settings", nullptr, &_openWindows["Bloom"]);
            ImGui::MenuItem("SSAO Settings", nullptr, &_openWindows["SSAO"]);
            ImGui::MenuItem("FXAA Settings", nullptr, &_openWindows["FXAA"]);
            ImGui::MenuItem("Fog Settings", nullptr, &_openWindows["Fog"]);
            ImGui::MenuItem("Tonemapping Settings", nullptr, &_openWindows["Tonemapping"]);
            ImGui::MenuItem("Lighting Settings", nullptr, &_openWindows["Lighting"]);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Editor"))
        {
            ImGui::MenuItem("Hierarchy", nullptr, &_openWindows["Hierarchy"]);

            ImGui::MenuItem("Entity editor", nullptr, &_openWindows["EntityEditor"]);

            ImGui::MenuItem("Particle editor", nullptr, &_openWindows["ParticleEditor"]);

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Systems"))
        {
            for (const auto& system : engine.GetModule<ECSModule>().GetSystems())
            {
                ImGui::MenuItem(system->GetName().data(), nullptr, &_openWindows[system->GetName().data()]);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Save Settings"))
        {
            engine.GetModule<RendererModule>().GetRenderer()->GetSettings().Write();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Exit Program"))
        {
            engine.SetExit(0);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (_openWindows["Hierarchy"])
    {
        _editor->DrawHierarchy();
    }

    if (_openWindows["EntityEditor"])
    {
        _editor->DrawEntityEditor();
    }

    if (_openWindows["ParticleEditor"])
    {
        _particleEditor->Render();
    }

    if (_openWindows["RenderStats"])
    {
        DrawRenderStats(engine);
    }

    if (_openWindows["Performance"])
    {
        _performanceTracker->Render();
    }

    if (_openWindows["Shadow Map"])
    {
        DrawShadowMapInspect(engine, *_imguiBackend);
    }

    Settings& settings = engine.GetModule<RendererModule>().GetRenderer()->GetSettingsData();

    if (_openWindows["Bloom"])
    {
        DrawBloomSettings(settings);
    }

    if (_openWindows["SSAO"])
    {
        DrawSSAOSettings(settings);
    }

    if (_openWindows["FXAA"])
    {
        DrawFXAASettings(settings);
    }

    if (_openWindows["Fog"])
    {
        DrawFogSettings(settings);
    }

    if (_openWindows["Tonemapping"])
    {
        DrawTonemappingSettings(settings);
    }

    if (_openWindows["Lighting"])
    {
        DrawLightingSettings(settings);
    }

    {
        ZoneNamedN(zz, "System inspect", true);
        for (const auto& system : engine.GetModule<ECSModule>().GetSystems())
        {
            if (_openWindows[system->GetName().data()])
            {
                system->Inspect();
            }
        }
    }
#endif

    {
        ZoneNamedN(zz, "ImGui Render", true);
        ImGui::Render();
    }
}

void DrawRenderStats(Engine& engine)
{
    const auto stats = engine.GetModule<RendererModule>().GetRenderer()->GetContext()->GetDrawStats();

    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("Renderer Stats", nullptr, ImGuiWindowFlags_NoResize);

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

void DrawBloomSettings(Settings& settings)
{
    auto& bloom = settings.bloom;

    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("Bloom Settings", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::InputFloat("Strength", &bloom.strength, 0.5f, 2.0f);
    ImGui::InputFloat("Gradient strength", &bloom.gradientStrength, 0.05f, 0.1f, "%.00005f");
    ImGui::InputFloat("Filter radius", &bloom.filterRadius, 0.005f, 0.02f, "%.0005f");
    ImGui::InputFloat("Max brightness extraction", &bloom.maxBrightnessExtraction, 1.0f, 5.0f);
    ImGui::InputFloat3("Color weights", &bloom.colorWeights[0], "%.00005f");
    ImGui::End();
}

void DrawSSAOSettings(Settings& settings)
{
    auto& ssao = settings.ssao;

    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("SSAO Settings", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::DragFloat("AO strength", &ssao.strength, 0.1f, 0.0f, 16.0f);
    ImGui::DragFloat("Bias", &ssao.bias, 0.001f, 0.0f, 0.1f);
    ImGui::DragFloat("Radius", &ssao.radius, 0.05f, 0.0f, 2.0f);
    ImGui::DragFloat("Minimum AO distance", &ssao.minDistance, 0.05f, 0.0f, 1.0f);
    ImGui::DragFloat("Maximum AO distance", &ssao.maxDistance, 0.05f, 0.0f, 1.0f);
    ImGui::End();
}

void DrawFXAASettings(Settings& settings)
{
    auto& fxaa = settings.fxaa;

    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("FXAA Settings", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::Checkbox("Enable FXAA", &fxaa.enableFXAA);
    ImGui::DragFloat("Edge treshold min", &fxaa.edgeThresholdMin, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Edge treshold max", &fxaa.edgeThresholdMax, 0.001f, 0.0f, 1.0f);
    ImGui::DragFloat("Subpixel quality", &fxaa.subPixelQuality, 0.01f, 0.0f, 1.0f);
    ImGui::DragInt("Iterations", &fxaa.iterations, 1, 1, 128);
    ImGui::End();
}

void DrawFogSettings(Settings& settings)
{
    auto& fog = settings.fog;

    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("Fog Settings", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::ColorPicker3("Color", &fog.color.x);
    ImGui::DragFloat("Density", &fog.density, 0.01f);
    ImGui::End();
}

void DrawTonemappingSettings(Settings& settings)
{
    auto& tonemapping = settings.tonemapping;

    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("Tonemapping Settings", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::SeparatorText("Tonemapping");
    ImGui::SliderFloat("Exposure", &tonemapping.exposure, 0.0f, 2.0f);

    int32_t value = static_cast<int32_t>(tonemapping.tonemappingFunction);
    int32_t i { 0 };
    for (const auto& name : magic_enum::enum_names<TonemappingFunctions>())
    {
        ImGui::RadioButton(name.data(), &value, i++);
    }
    tonemapping.tonemappingFunction = static_cast<TonemappingFunctions>(value);

    ImGui::Checkbox("##V", &tonemapping.enableVignette);
    ImGui::SameLine();
    ImGui::SeparatorText("Vignette");
    ImGui::BeginDisabled(!tonemapping.enableVignette);
    ImGui::SliderFloat("Intensity##V", &tonemapping.vignetteIntensity, 0.0f, 2.0f);
    ImGui::EndDisabled();

    ImGui::Checkbox("##LD", &tonemapping.enableLensDistortion);
    ImGui::SameLine();
    ImGui::SeparatorText("Lens Distortion");
    ImGui::BeginDisabled(!tonemapping.enableLensDistortion);
    ImGui::SliderFloat("Intensity##LD", &tonemapping.lensDistortionIntensity, -2.0f, 2.0f);
    ImGui::SliderFloat("Cubic Intensity", &tonemapping.lensDistortionCubicIntensity, -2.0f, 2.0f);
    ImGui::SliderFloat("Screen Scale", &tonemapping.screenScale, 0.0f, 2.0f);
    ImGui::EndDisabled();

    ImGui::Checkbox("##Tone", &tonemapping.enableToneAdjustments);
    ImGui::SameLine();
    ImGui::SeparatorText("Tone");
    ImGui::BeginDisabled(!tonemapping.enableToneAdjustments);
    ImGui::DragFloat("Brightness", &tonemapping.brightness, 0.005f);
    ImGui::DragFloat("Contrast", &tonemapping.contrast, 0.005f);
    ImGui::DragFloat("Saturation", &tonemapping.saturation, 0.005f);
    ImGui::DragFloat("Vibrance", &tonemapping.vibrance, 0.005f);
    ImGui::SliderFloat("Hue", &tonemapping.hue, 0.0f, 1.0f);
    ImGui::EndDisabled();

    // Pixelization
    ImGui::Checkbox("##Pixelization", &tonemapping.enablePixelization);
    ImGui::SameLine();
    ImGui::SeparatorText("Pixelization");
    ImGui::BeginDisabled(!tonemapping.enablePixelization);
    ImGui::DragFloat("Min Pixel Size", &tonemapping.minPixelSize, 0.01f);
    ImGui::DragFloat("Max Pixel Size", &tonemapping.maxPixelSize, 0.01f);
    ImGui::DragFloat("Pixelization Levels", &tonemapping.pixelizationLevels, 0.01f);
    ImGui::DragFloat("Pixelization Depth Bias", &tonemapping.pixelizationDepthBias, 1.0f);
    ImGui::EndDisabled();

    // Fixed palette
    ImGui::Checkbox("##Color Palette", &tonemapping.enablePalette);
    ImGui::SameLine();
    ImGui::SeparatorText("Color Palette");
    ImGui::BeginDisabled(!tonemapping.enablePalette);
    ImGui::DragFloat("Dither Amount", &tonemapping.ditherAmount, 0.01f);
    ImGui::DragFloat("Palette Amount", &tonemapping.paletteAmount, 0.01f);

    // Iterate over the palette vector
    for (size_t i = 0; i < tonemapping.palette.size();)
    {
        // Push a unique ID so that ImGui can differentiate each item
        ImGui::PushID(static_cast<int>(i));

        // Display a color editor for the current palette color
        // Casting the glm::vec4 pointer to a float pointer is safe if glm::vec4 is 4 floats.
        ImGui::ColorEdit4("Color", &tonemapping.palette[i].x, ImGuiColorEditFlags_NoInputs);

        ImGui::SameLine();

        // "X" button for removal
        if (ImGui::Button("X"))
        {
            // Erase the element at the current index and continue without incrementing i,
            // since the next element shifts into the current index.
            tonemapping.palette.erase(tonemapping.palette.begin() + i);
            ImGui::PopID();
            continue;
        }

        ImGui::PopID();
        i++; // Only increment when no deletion occurs
    }

    // A separator for clarity
    ImGui::Separator();

    // Button to add the new color to the palette
    if (ImGui::Button("Add Color"))
    {
        tonemapping.palette.push_back(glm::vec4(1.0f));
    }
    ImGui::EndDisabled();

    ImGui::SeparatorText("Procedural Sky");
    ImGui::ColorEdit3("Sky Color", &tonemapping.skyColor.x);
    ImGui::ColorEdit3("Sun Color", &tonemapping.sunColor.x);
    ImGui::ColorEdit3("Clouds Color", &tonemapping.cloudsColor.x);
    ImGui::ColorEdit3("Void Color", &tonemapping.voidColor.x);
    ImGui::DragFloat("Clouds Speed", &tonemapping.cloudsSpeed);

    ImGui::End();
}

void DrawLightingSettings(Settings& settings)
{
    auto& lighting = settings.lighting;

    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("Lighting Settings", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::DragFloat("Ambient Strength", &lighting.ambientStrength, 0.01f, 0.0f, 16.0f);
    ImGui::DragFloat("Ambient Shadow Strength", &lighting.ambientShadowStrength, 0.01f, 0.0f, 1.0f);
    ImGui::DragFloat("Decals Normal Wrap Threshold", &lighting.decalNormalThreshold, 0.1f, 0.0f, 180.0f);

    ImGui::End();
}

void DrawShadowMapInspect(Engine& engine, ImGuiBackend& imguiBackend)
{
    static ImTextureID textureID = imguiBackend.GetTexture(engine.GetModule<RendererModule>().GetRenderer()->GetGPUScene().StaticShadow());
    static ImTextureID textureID2 = imguiBackend.GetTexture(engine.GetModule<RendererModule>().GetRenderer()->GetGPUScene().DynamicShadow());
    ImGui::SetNextWindowSize({ 0.f, 0.f });
    ImGui::Begin("Directional Light Shadow Map View", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::Image(textureID, ImVec2(512, 512));
    ImGui::Image(textureID2, ImVec2(512, 512));
    if (ImGui::Button("Force recalculate shadow map"))
    {
        auto view = engine.GetModule<ECSModule>().GetRegistry().view<StaticMeshComponent>();
        for (auto entity : view)
        {
            engine.GetModule<ECSModule>().GetRegistry().emplace_or_replace<WantsShadowsUpdated>(entity);
        }
    }
    ImGui::End();
}

void SetupImGuiStyle()
{
    // Everforest style by DestroyerDarkNess from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6000000238418579f;
    style.WindowPadding = ImVec2(6.0f, 3.0f);
    style.WindowRounding = 6.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 0.0f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(5.0f, 1.0f);
    style.FrameRounding = 3.0f;
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 13.0f;
    style.ScrollbarRounding = 16.0f;
    style.GrabMinSize = 20.0f;
    style.GrabRounding = 2.0f;
    style.TabRounding = 4.0f;
    style.TabBorderSize = 1.0f;
    style.TabMinWidthForCloseButton = 0.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(0.8745098114013672f, 0.8705882430076599f, 0.8392156958580017f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.5843137502670288f, 0.572549045085907f, 0.5215686559677124f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 0.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.5960784554481506f, 0.5921568870544434f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5960784554481506f, 0.5921568870544434f, 0.1019607856869698f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.4000000059604645f, 0.3607843220233917f, 0.3294117748737335f, 1.0f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.4862745106220245f, 0.43529412150383f, 0.3921568691730499f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 0.9725490212440491f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.3137255012989044f, 0.2862745225429535f, 0.2705882489681244f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.7411764860153198f, 0.7176470756530762f, 0.4196078479290009f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 0.6094420552253723f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 0.4313725531101227f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.8392156958580017f, 0.7490196228027344f, 0.4000000059604645f, 0.9019607901573181f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2352941185235977f, 0.2196078449487686f, 0.2117647081613541f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
}
