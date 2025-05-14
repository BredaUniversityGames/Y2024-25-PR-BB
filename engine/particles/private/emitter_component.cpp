#include "emitter_component.hpp"
#include "particle_module.hpp"

#include <magic_enum.hpp>

namespace EnttEditor
{
template <>
void ComponentEditorWidget<ParticleEmitterComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& t = reg.get<ParticleEmitterComponent>(e);
    t.Inspect();
}
}
void ParticleEmitterComponent::Inspect()
{
    // component variables
    ImGui::Checkbox("Emit once##Particle Emitter", &emitOnce);
    ImGui::DragFloat("Emit delay##Particle Emitter", &maxEmitDelay, 0.0f, 50.0f);

    // emitter variables
    ImGui::Text("Emitter");
    ImGui::DragFloat3("Position##Particle Emitter", &emitter.position.x, 0.1f);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
    {
        ImGui::TextUnformatted("Only applied when there is no rigidBody or transform.");
        ImGui::EndTooltip();
    }
    ImGui::DragFloat3("Position Offset##Particle Emitter", &positionOffset.x, 0.1f);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
    {
        ImGui::TextUnformatted("Offset to the position of the entity's rigidBody or transform.");
        ImGui::EndTooltip();
    }
    ImGui::DragFloat("Frame Rate##Emitter Preset", &emitter.frameRate, 0.0f, 50.0f);
    int32_t emitterCount = static_cast<int32_t>(count);
    ImGui::DragInt("Count##Particle Emitter", &emitterCount, 1, 0, 1024);
    count = static_cast<uint32_t>(emitterCount);
    ImGui::DragFloat3("Velocity##Particle Emitter", &emitter.velocity.x, 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat("Mass##Particle Emitter", &emitter.mass, 1.0f, -100.0f, 100.0f);
    ImGui::DragFloat("Rotation##Particle Emitter", &emitter.rotationVelocity.x, 1.0f, -100.0f, 100.0f);
    ImGui::DragFloat("Rotation velocity##Particle Emitter", &emitter.rotationVelocity.y, 1.0f, -100.0f, 100.0f);
    ImGui::DragFloat2("Size##Particle Emitter", &emitter.size.x, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Size velocity##Particle Emitter", &emitter.size.z, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Max life##Particle Emitter", &emitter.maxLife, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat3("Spawn Randomness##EmitterPresetEditor", &emitter.spawnRandomness.x, 0.1f, 0.0f, 100.0f);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && ImGui::BeginTooltip())
    {
        ImGui::TextUnformatted("Adjusts how much the initial velocity of the spawned particles is randomized");
        ImGui::TextUnformatted("on the x, y and z axis.");
        ImGui::Spacing();
        ImGui::TextUnformatted("Formula:");
        ImGui::TextUnformatted("particle.velocity = emitter.velocity + noise3DValue * emitter.spawnRandomness");

        ImGui::EndTooltip();
    }
    ImGui::DragFloat3("Velocity Randomness##EmitterPresetEditor", &emitter.velocityRandomness.x, 0.1f, 0.0f, 100.0f);
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
    ImGui::ColorPicker3("Color##Particle Emitter", &emitter.color.x);
    ImGui::DragFloat("Color Multiplier##Particle Emitter", &emitter.color.w, 0.1f, 0.0f, 100.0f);

    // flag dropdown
    ImGui::Text("Rendering Flags:");
    uint32_t flags = emitter.flags;
    ImGui::CheckboxFlags("Unlit##Emitter Preset Flag", &flags, static_cast<uint32_t>(ParticleRenderFlagBits::eUnlit));
    ImGui::CheckboxFlags("No Shadow##Emitter Preset Flag", &flags, static_cast<uint32_t>(ParticleRenderFlagBits::eNoShadow));
    ImGui::CheckboxFlags("Frame Blend##Emitter Preset Flag", &flags, static_cast<uint32_t>(ParticleRenderFlagBits::eFrameBlend));
    ImGui::CheckboxFlags("Lock Y##Emitter Preset Flag", &flags, static_cast<uint32_t>(ParticleRenderFlagBits::eLockY));
    emitter.flags = flags;

    ImGui::Text("Bursts:");
    if (ImGui::BeginTable("Bursts##Particle Emitter", 7, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuter))
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

        for (auto it = bursts.begin(); it != bursts.end();)
        {
            int32_t index = std::distance(bursts.begin(), it);

            auto copyIt = it;
            it++;
            auto& burst = *copyIt;

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Burst %i", index);

            ImGui::TableNextColumn();
            int32_t burstCount = static_cast<int32_t>(burst.count);
            ImGui::DragInt(std::string("##Emitter Burst Count " + std::to_string(index)).c_str(), &burstCount);
            burst.count = static_cast<uint32_t>(burstCount);

            ImGui::TableNextColumn();
            ImGui::DragFloat(std::string("##Emitter Burst Start Time " + std::to_string(index)).c_str(), &burst.startTime, 0.1f, 0.0f, 100.0f);

            ImGui::TableNextColumn();
            ImGui::DragFloat(std::string("##Emitter Burst Max Interval " + std::to_string(index)).c_str(), &burst.maxInterval, 0.1f, 0.0f, 100.0f);

            ImGui::TableNextColumn();
            ImGui::Checkbox(std::string("##Emitter Burst Loop " + std::to_string(index)).c_str(), &burst.loop);

            ImGui::TableNextColumn();
            int32_t burstCycles = static_cast<int32_t>(burst.cycles);
            ImGui::DragInt(std::string("##Emitter Burst Cycles " + std::to_string(index)).c_str(), &burstCycles);
            burst.cycles = static_cast<uint32_t>(burstCycles);

            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.2f, 0.2f, 1.f));
            if (ImGui::Button(std::string("x##Emitter Burst Remove " + std::to_string(index)).c_str()))
            {
                bursts.erase(copyIt);
            }
            ImGui::PopStyleColor(3);
        }
        ImGui::EndTable();
    }
    if (ImGui::Button("+ Add Burst##Emitter Burst Add"))
    {
        bursts.emplace_back(ParticleBurst());
    }
}