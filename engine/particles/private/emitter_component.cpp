#include "emitter_component.hpp"

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
    int32_t emitterCount = static_cast<int32_t>(emitter.count);
    ImGui::DragInt("Count##Particle Emitter", &emitterCount, 1, 0, 1024);
    emitter.count = static_cast<uint32_t>(emitterCount);
    ImGui::DragFloat3("Velocity##Particle Emitter", &emitter.velocity.x, 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat("Mass##Particle Emitter", &emitter.mass, 1.0f, -100.0f, 100.0f);
    ImGui::DragFloat2("Rotation velocity##Particle Emitter", &emitter.rotationVelocity.x, 1.0f, -100.0f, 100.0f);
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
    ImGui::DragFloat("Color Multiplier#Particle Emitter", &emitter.color.w, 0.1f, 0.0f, 100.0f);
}