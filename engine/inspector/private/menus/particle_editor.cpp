#include "particle_editor.hpp"

#include "components/camera_component.hpp"
#include "components/name_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "gpu_resources.hpp"

#include "imgui/imgui.h"
#include <magic_enum.hpp>
#include <misc/cpp/imgui_stdlib.h>

ParticleEditor::ParticleEditor(ParticleModule& particleModule, ECSModule& ecsModule)
    : _particleModule(particleModule)
    , _ecsModule(ecsModule)
{
}

void ParticleEditor::Render()
{
    ImGui::Begin("Particle preset editor", nullptr);
    auto region = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("List##ParticlePresetEditor", { 150, region.y }, true))
    {
        RenderEmitterPresetList();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("Editor##ParticlePresetEditor", { region.x - 150, region.y }, true))
    {
        RenderEmitterPresetEditor();
    }
    ImGui::EndChild();

    ImGui::End();
}

void ParticleEditor::RenderEmitterPresetList()
{
    if (ImGui::Button("+ new preset##EmitterPresetEditor"))
    {
        ParticleModule::EmitterPreset newPreset;
        newPreset.name += " " + std::to_string(_particleModule._emitterPresets.size());
        _particleModule._emitterPresets.emplace_back(std::move(newPreset));
    }
    ImGui::Text("Emitter Presets:");

    for (uint32_t i = 0; i < _particleModule._emitterPresets.size(); i++)
    {
        auto& emitterPreset = _particleModule._emitterPresets[i];
        static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        ImGui::TreeNodeEx(emitterPreset.name.c_str(), nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked())
        {
            _selectedPresetIndex = i;
            _imageLoadMessage = "Ready to load...";
        }
    }
}

void ParticleEditor::RenderEmitterPresetEditor()
{
    ImGui::Text("Currently editing: ");
    ImGui::SameLine();

    if (_selectedPresetIndex > -1)
    {
        auto& selectedPreset = _particleModule._emitterPresets[_selectedPresetIndex];

        ImGui::InputText("Name##EmitterPresetEditor", &selectedPreset.name);

        // image loading (scuffed for now)
        ImGui::Text("assets/textures/");
        ImGui::SameLine();
        ImGui::InputText("Image##EmitterPresetEditor", &_currentImage);

        if (ImGui::Button("Load Image##EmitterPresetEditor"))
        {
            auto image = _particleModule.GetEmitterImage(_currentImage);
            _particleModule.SetEmitterPresetImage(selectedPreset, image);
            _imageLoadMessage = "Loaded successfully!";
        }
        ImGui::SameLine();
        ImGui::Text("%s", _imageLoadMessage.c_str());

        // parameter editors
        ImGui::DragFloat("Emit delay##EmitterPresetEditor", &selectedPreset.emitDelay, 0.0f, 50.0f);
        int emitterCount = static_cast<int>(selectedPreset.count);
        ImGui::DragInt("Count##EmitterPresetEditor", &emitterCount, 1, 0, 1024);
        selectedPreset.count = emitterCount;
        ImGui::DragFloat("Mass##EmitterPresetEditor", &selectedPreset.mass, 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat2("Rotation velocity##EmitterPresetEditor", &selectedPreset.rotationVelocity.x, 1.0f, -100.0f, 100.0f);
        ImGui::DragFloat2("Size##EmitterPresetEditor", &selectedPreset.size.x, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Size velocity##EmitterPresetEditor", &selectedPreset.size.z, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Max life##EmitterPresetEditor", &selectedPreset.maxLife, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat3("Randomness##EmitterPresetEditor", &selectedPreset.randomness.x, 0.1f, 0.0f, 100.0f);
        ImGui::ColorPicker3("Color##EmitterPresetEditor", &selectedPreset.color.x);
        ImGui::DragFloat("Color Multiplier#EmitterPresetEditor", &selectedPreset.color.w, 0.1f, 0.0f, 100.0f);

        if (ImGui::Button("Spawn##EmitterPresetEditor"))
        {
            entt::entity entity = _ecsModule.GetRegistry().create();
            NameComponent node;
            node.name = "Particle Emitter";
            _ecsModule.GetRegistry().emplace<NameComponent>(entity, node);

            TransformComponent transform;
            _ecsModule.GetRegistry().emplace<TransformComponent>(entity, transform);
            const auto view = _ecsModule.GetRegistry().view<CameraComponent, TransformComponent>();
            for (const auto cameraEntity : view)
            {
                const auto& cameraTransform = _ecsModule.GetRegistry().get<TransformComponent>(cameraEntity);
                TransformHelpers::SetLocalPosition(_ecsModule.GetRegistry(), entity, cameraTransform.GetLocalPosition());
            }
            _particleModule.SpawnEmitter(entity, _selectedPresetIndex, SpawnEmitterFlagBits::eIsActive | SpawnEmitterFlagBits::eSetCustomVelocity, { 8.0f, 35.0f, 300.0f });
        }

        ImGui::SameLine();

        // red button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.2f, 0.2f, 1.f));
        if (ImGui::Button("Delete##EmitterPresetEditor"))
        {
            _particleModule._emitterPresets.erase(_particleModule._emitterPresets.begin() + _selectedPresetIndex);
            _selectedPresetIndex = -1;
            ImGui::PopStyleColor(3);
            return;
        }
        ImGui::PopStyleColor(3);
    }
    else
    {
        ImGui::Text("No preset selected");
    }
}
