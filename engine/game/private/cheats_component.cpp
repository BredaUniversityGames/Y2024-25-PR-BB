#include "cheats_component.hpp"

namespace EnttEditor
{
template <>
void ComponentEditorWidget<CheatsComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<CheatsComponent>(e);
    ImGui::Checkbox("Noclip##CheatsComponent", &comp.noClip);
}
}