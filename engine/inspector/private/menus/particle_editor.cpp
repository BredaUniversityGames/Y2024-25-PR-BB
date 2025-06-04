#include "particle_editor.hpp"

#include "components/camera_component.hpp"
#include "components/name_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "game_module.hpp"
#include "resource_management/image_resource_manager.hpp"

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
        EmitterPreset newPreset;
        std::string newPresetName = "Emiter Preset " + std::to_string(_particleModule._emitterPresets.data.emitterPresets.size());
        _particleModule._emitterPresets2.data.emitterPresets.emplace(newPresetName, newPreset);
    }
    if (ImGui::Button("Save Presets##Emitter Preset"))
    {
        _particleModule._emitterPresets.Write();
    }
    ImGui::Text("Emitter Presets:");

    for (auto& it : _particleModule._emitterPresets2.data.emitterPresets)
    {
        static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

        ImGui::TreeNodeEx(it.first.c_str(), nodeFlags | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked())
        {
            _selectedPresetName = it.first.c_str();
            _selectedPresetEditingName = it.first.c_str();

            _imageLoadMessage = "Ready to load...";
            _nameChangeMessage = "";
        }
    }
}

void ParticleEditor::RenderEmitterPresetEditor()
{
    ImGui::Text("Currently editing: ");
    ImGui::SameLine();
    ImGui::Text("%s", _selectedPresetName.c_str());

    auto got = _particleModule._emitterPresets2.data.emitterPresets.find(_selectedPresetName);

    if (got != _particleModule._emitterPresets2.data.emitterPresets.end())
    {
        auto& selectedPreset = got->second;

        ImGui::InputText("Name##Emitter Preset", &_selectedPresetEditingName);
        if (ImGui::Button("Save Name##Emitter Preset"))
        {
            if (_particleModule._emitterPresets2.data.emitterPresets.find(_selectedPresetEditingName) == _particleModule._emitterPresets2.data.emitterPresets.end())
            {
                auto preset = _particleModule._emitterPresets2.data.emitterPresets.extract(_selectedPresetName);
                preset.key() = _selectedPresetEditingName;
                _selectedPresetName = _selectedPresetEditingName;
                _particleModule._emitterPresets2.data.emitterPresets.insert(std::move(preset));
                _nameChangeMessage = "Name successfully changed!";
            }
            else
            {
                _nameChangeMessage = "Name already exists :(";
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s", _nameChangeMessage.c_str());

        // image loading (scuffed for now)
        ImGui::Text("assets/textures/particles/");
        ImGui::SameLine();
        ImGui::InputText("Image##Emitter Preset", &selectedPreset.imageName);

        if (ImGui::Button("Load Image##Emitter Preset"))
        {
            if (_particleModule.SetEmitterPresetImage(selectedPreset))
            {
                _imageLoadMessage = "Loaded successfully!";
            }
            else
            {
                _imageLoadMessage = "Error loading image...";
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s", _imageLoadMessage.c_str());
        ImGui::DragInt2("Sprite Dimensions##Emitter Preset", &selectedPreset.spriteDimensions.x);
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted("Manually specify the amount of frames that can be present over the x and y axis in the loaded image.");
            ImGui::EndTooltip();
        }
        int frameCount = static_cast<int>(selectedPreset.frameCount);
        ImGui::DragInt("Frame Count##Emitter Preset", &frameCount);
        selectedPreset.frameCount = frameCount;
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted("Manually specify the amount of frames present in the loaded image.");
            ImGui::EndTooltip();
        }

        // parameter editors
        ImGui::DragFloat("Frame Rate##Emitter Preset", &selectedPreset.frameRate, 0.0f, 50.0f);
        ImGui::DragFloat("Emit delay##Emitter Preset", &selectedPreset.emitDelay, 0.0f, 50.0f);
        int emitterCount = static_cast<int>(selectedPreset.count);
        ImGui::DragInt("Count##Emitter Preset", &emitterCount, 1, 0, 1024);
        selectedPreset.count = emitterCount;
        ImGui::DragFloat3("Starting Velocity##Emitter Preset", &selectedPreset.startingVelocity.x, 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat("Mass##Emitter Preset", &selectedPreset.mass, 0.1f, -100.0f, 100.0f);
        ImGui::DragFloat("Rotation##Particle Emitter", &selectedPreset.rotationVelocity.x, 1.0f, -100.0f, 100.0f);
        ImGui::DragFloat("Rotation velocity##Emitter Preset", &selectedPreset.rotationVelocity.y, 1.0f, -100.0f, 100.0f);
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

        // flag dropdown
        ImGui::Text("Rendering Flags:");
        ImGui::CheckboxFlags("Unlit##Emitter Preset Flag", &selectedPreset.flags, static_cast<uint32_t>(ParticleRenderFlagBits::eUnlit));
        ImGui::CheckboxFlags("No Shadow##Emitter Preset Flag", &selectedPreset.flags, static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow));
        ImGui::CheckboxFlags("Frame Blend##Emitter Preset Flag", &selectedPreset.flags, static_cast<uint32_t>(ParticleRenderFlagBits::eFrameBlend));
        ImGui::CheckboxFlags("Lock Y##Emitter Preset Flag", &selectedPreset.flags, static_cast<uint32_t>(ParticleRenderFlagBits::eLockY));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted("Makes this emitter's particle billboards only rotate around the Y axis.");

            ImGui::EndTooltip();
        }
        ImGui::CheckboxFlags("Is Local##Emitter Preset Flag", &selectedPreset.flags, static_cast<uint32_t>(ParticleRenderFlagBits::eIsLocal));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
        {
            ImGui::TextUnformatted("Makes this emitter's particles follow its emitter's position");
            ImGui::TextUnformatted("instead of simulating their own.");

            ImGui::EndTooltip();
        }

        // burst table
        ImGui::Text("Bursts:");
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
                int32_t index = std::distance(selectedPreset.bursts.begin(), it);
                ImGui::TableNextRow();

                auto copyIt = it;
                it++;
                auto& burst = *copyIt;

                ImGui::TableNextColumn();
                ImGui::Text("Burst %i", index);

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
            const auto view = _ecsModule.GetRegistry().view<PlayerTag, TransformComponent>();
            for (const auto viewEntity : view)
            {
                const auto& playerTransform = _ecsModule.GetRegistry().get<TransformComponent>(viewEntity);
                TransformHelpers::SetLocalPosition(_ecsModule.GetRegistry(), entity, playerTransform.GetLocalPosition());
            }
            _ecsModule.GetRegistry().emplace<TestEmitterTag>(entity);
            _particleModule.SpawnEmitter(entity, _selectedPresetName, SpawnEmitterFlagBits::eIsActive);
        }

        ImGui::SameLine();

        // red button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.2f, 0.2f, 1.f));
        if (ImGui::Button("Delete Preset##Emitter Preset"))
        {
            _particleModule._emitterPresets2.data.emitterPresets.erase(got);
            _selectedPresetName = "null";
            _selectedPresetEditingName = "null";
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
