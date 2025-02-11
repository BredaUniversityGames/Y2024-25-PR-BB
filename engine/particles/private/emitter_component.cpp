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
    ImGui::Checkbox("Emit once##Particle Emitter", &emitOnce);
    ImGui::DragFloat("Emit delay##Particle Emitter", &maxEmitDelay, 0.0f, 50.0f);

    constexpr auto types = magic_enum::enum_names<ParticleType>();
    static auto currentType = types[0];
    if (ImGui::BeginCombo("Type##Particle Emitter", std::string(currentType).c_str()))
    {
        for (uint32_t n = 0; n < types.size(); n++)
        {
            bool isSelected = (currentType == types[n]);
            if (ImGui::Selectable(std::string(types[n]).c_str(), isSelected))
            {
                currentType = types[n];
                type = static_cast<ParticleType>(n);
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus(); // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Emitter");
    ImGui::DragFloat3("Position##Particle Emitter", &emitter.position.x, 0.1f);
    int32_t emitterCount = static_cast<int32_t>(emitter.count);
    ImGui::DragInt("Count##Particle Emitter", &emitterCount, 1, 0, 1024);
    emitter.count = static_cast<uint32_t>(emitterCount);
    ImGui::DragFloat3("Velocity##Particle Emitter", &emitter.velocity.x, 0.1f, -100.0f, 100.0f);
    ImGui::DragFloat3("Randomness##EmitterPresetEditor", &emitter.randomness.x, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat("Mass##Particle Emitter", &emitter.mass, 1.0f, -100.0f, 100.0f);
    ImGui::DragFloat2("Rotation velocity##Particle Emitter", &emitter.rotationVelocity.x, 1.0f, -100.0f, 100.0f);
    ImGui::DragFloat("Max life##Particle Emitter", &emitter.maxLife, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat3("Size##Particle Emitter", &emitter.size.x, 0.1f, 0.0f, 100.0f);
    ImGui::DragFloat3("Color Multiplier##Particle Emitter", &emitter.color.x, 0.1f, 0.0f, 100.0f);
}