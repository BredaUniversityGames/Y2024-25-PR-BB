#include "lifetime_component.hpp"

namespace EnttEditor
{
template <>
void ComponentEditorWidget<LifetimeComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<LifetimeComponent>(e);
    ImGui::InputFloat("Lifetime##LifetimeComponent", &comp.lifetime);
}
}