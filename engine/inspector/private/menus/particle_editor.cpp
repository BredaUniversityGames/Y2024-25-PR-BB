#include "particle_editor.hpp"

#include "components/camera_component.hpp"
#include "components/name_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "gpu_resources.hpp"

#include "imgui.h"
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
    if (ImGui::BeginChild("List##Emitter Preset", { 150, region.y }, true))
    {
        RenderEmitterPresetList();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    if (ImGui::BeginChild("Editor##Emitter Preset", { region.x - 150, region.y }, true))
    {
        RenderEmitterPresetEditor();
    }
    ImGui::EndChild();

    ImGui::End();
}

void ParticleEditor::RenderEmitterPresetList()
{
    if (ImGui::Button("+ New Preset##Emitter Preset"))
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

        ImGui::InputText("Name##Emitter Preset", &selectedPreset.name);

        // image loading (scuffed for now)
        ImGui::Text("assets/textures/");
        ImGui::SameLine();
        ImGui::InputText("Image##Emitter Preset", &_currentImage);

        if (ImGui::Button("Load Image##Emitter Preset"))
        {
            auto image = _particleModule.GetEmitterImage(_currentImage);
            _particleModule.SetEmitterPresetImage(selectedPreset, image);
            _imageLoadMessage = "Loaded successfully!";
        }
        ImGui::SameLine();
        ImGui::Text("%s", _imageLoadMessage.c_str());

        // parameter editors
        ImGui::DragFloat("Emit delay##Emitter Preset", &selectedPreset.emitDelay, 0.0f, 50.0f);
        int emitterCount = static_cast<int>(selectedPreset.count);
        ImGui::DragInt("Count##Emitter Preset", &emitterCount, 1, 0, 1024);
        selectedPreset.count = emitterCount;
        ImGui::DragFloat("Mass##Emitter Preset", &selectedPreset.mass, 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat2("Rotation velocity##Emitter Preset", &selectedPreset.rotationVelocity.x, 1.0f, -100.0f, 100.0f);
        ImGui::DragFloat2("Size##Emitter Preset", &selectedPreset.size.x, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Size velocity##Emitter Preset", &selectedPreset.size.z, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Max life##Emitter Preset", &selectedPreset.maxLife, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat3("Spawn Randomness##Emitter Preset", &selectedPreset.spawnRandomness.x, 0.1f, 0.0f, 100.0f);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted("Adjusts how much the initial velocity of the spawned particles is randomized");
            ImGui::TextUnformatted("on the x, y and z axis.");
            ImGui::Spacing();
            ImGui::TextUnformatted("Formula:");
            ImGui::TextUnformatted("particle.velocity = emitter.velocity + noise3DValue * emitter.spawnRandomness");

            ImGui::EndTooltip();
        }
        ImGui::DragFloat3("Velocity Randomness##Emitter Preset", &selectedPreset.velocityRandomness.x, 0.1f, 0.0f, 100.0f);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted("Adjusts how much the velocity of the particles during simulation is randomized");
            ImGui::TextUnformatted("on the x, y and z axis.");
            ImGui::Spacing();
            ImGui::TextUnformatted("Formula:");
            ImGui::TextUnformatted("particle.velocity += noise3DValue * particle.velocityRandomness");
            ImGui::TextUnformatted("particle.position += particle.velocity * deltaTime");

            ImGui::EndTooltip();
        }
        ImGui::ColorPicker3("Color##Emitter Preset", &selectedPreset.color.x);
        ImGui::DragFloat("Color Multiplier##Emitter Preset", &selectedPreset.color.w, 0.1f, 0.0f, 100.0f);

        if (ImGui::BeginTable("Bursts##Emitter Preset", 7, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuter))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::Text("Count");
            ImGui::TableNextColumn();
            ImGui::Text("Start Time");
            ImGui::TableNextColumn();
            ImGui::Text("Max Interval");
            ImGui::TableNextColumn();
            ImGui::Text("Loop");
            ImGui::TableNextColumn();
            ImGui::Text("Cycle");
            ImGui::TableNextColumn();

            for (auto it = selectedPreset.bursts.begin(); it != selectedPreset.bursts.end();)
            {
                std::size_t index = std::distance(selectedPreset.bursts.begin(), it);
                ImGui::TableNextRow();
                auto copyIt = it;
                it++;
                auto& burst = *copyIt;

                ImGui::TableNextColumn();
                ImGui::Text(std::string("Burst " + std::to_string(index)).c_str());

                ImGui::TableNextColumn();
                int32_t burstCount = static_cast<int32_t>(burst.count);
                ImGui::DragInt(std::string("##Preset Burst Count " + std::to_string(index)).c_str(), &burstCount);
                burst.count = static_cast<uint32_t>(burstCount);

                ImGui::TableNextColumn();
                ImGui::DragFloat(std::string("##Preset Burst Start Time " + std::to_string(index)).c_str(), &burst.startTime, 0.1f, 0.0f, 100.0f);

                ImGui::TableNextColumn();
                ImGui::DragFloat(std::string("##Preset Burst Max Interval " + std::to_string(index)).c_str(), &burst.maxInterval, 0.1f, 0.0f, 100.0f);

                ImGui::TableNextColumn();
                ImGui::Checkbox(std::string("##Preset Burst Loop " + std::to_string(index)).c_str(), &burst.loop);

                ImGui::TableNextColumn();
                int32_t burstCycles = static_cast<int32_t>(burst.cycles);
                ImGui::DragInt(std::string("##Preset Burst Cycles " + std::to_string(index)).c_str(), &burstCycles);
                burst.cycles = static_cast<uint32_t>(burstCycles);

                ImGui::TableNextColumn();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.2f, 0.2f, 1.f));
                if (ImGui::Button(std::string("x##Preset Burst Remove " + std::to_string(index)).c_str()))
                {
                    selectedPreset.bursts.erase(copyIt);
                }
                ImGui::PopStyleColor(3);
            }
            ImGui::EndTable();
        }
        if (ImGui::Button("+ Add Burst##Preset Burst Add"))
        {
            selectedPreset.bursts.emplace_back(ParticleBurst());
        }

        if (ImGui::Button("Spawn Emitter##Emitter Preset"))
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
        if (ImGui::Button("Delete Preset##Emitter Preset"))
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
