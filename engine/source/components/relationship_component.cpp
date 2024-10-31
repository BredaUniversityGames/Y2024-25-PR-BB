#include "components/relationship_component.hpp"

namespace EnttEditor
{
template <>
void ComponentEditorWidget<RelationshipComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<RelationshipComponent>(e);

    ImGui::Text("Children Count: %d", static_cast<int>(e));
    ImGui::Text("Parent: %d", static_cast<int>(comp.parent));
    ImGui::Text("First: %d", static_cast<int>(comp.first));
    ImGui::Text("Prev: %d", static_cast<int>(comp.prev));
    ImGui::Text("Next: %d", static_cast<int>(comp.next));
}
}