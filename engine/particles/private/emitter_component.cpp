#include "emitter_component.hpp"

namespace EnttEditor
{
template <>
void ComponentEditorWidget<EmitterComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& t = reg.get<EmitterComponent>(e);
    t.Inspect(reg, e);
}
}
void EmitterComponent::Inspect(entt::registry& reg, entt::entity entity)
{
    ImGui::Checkbox("EmitOnce##Particle Emitter", &emitOnce);
    ImGui::SliderFloat("MaxEmitDelay##Particle Emitter", &maxEmitDelay, 0.0f, 50.0f);

    const char* types[] = { "Billboard", "Ribbon" };
    static const char* currentType = types[0];
    if (ImGui::BeginCombo("Type##Particle Emitter", currentType))
    {
        for (int n = 0; n < IM_ARRAYSIZE(types); n++)
        {
            bool is_selected = (currentType == types[n]);
            if (ImGui::Selectable(types[n], is_selected))
            {
                currentType = types[n];
                type = static_cast<ParticleType>(n);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus(); // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Text("Emitter");
    ImGui::DragFloat3("Position##Particle Emitter", &emitter.position.x);
    int emitterCount = static_cast<int>(emitter.count);
    ImGui::SliderInt("Count##Particle Emitter", &emitterCount, 0, 1024);
    emitter.count = emitterCount;
    ImGui::DragFloat3("Velocity##Particle Emitter", &emitter.velocity.x);
    ImGui::SliderFloat("Mass##Particle Emitter", &emitter.mass, -100.0f, 100.0f);
    ImGui::DragFloat2("Rotation Velocity##Particle Emitter", &emitter.rotationVelocity.x);
    ImGui::SliderFloat("Max Life##Particle Emitter", &emitter.maxLife, 0.0f, 100.0f);
    ImGui::DragFloat3("Size##Particle Emitter", &emitter.size.x);
}