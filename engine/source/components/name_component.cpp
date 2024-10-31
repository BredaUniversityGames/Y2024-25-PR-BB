#include "components/name_component.hpp"

#include <entt/entity/registry.hpp>
#include "imgui/misc/cpp/imgui_stdlib.h"

std::string_view NameComponent::GetDisplayName(const entt::registry& registry, entt::entity entity)
{
    if (auto* ptr = registry.try_get<NameComponent>(entity))
    {
        return std::string_view { ptr->_name };
    }

    return std::string_view { "Unnamed Entity" };
}
namespace EnttEditor
{
template <>
void ComponentEditorWidget<NameComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<NameComponent>(e);
    ImGui::InputText("Name##NameComponent", &comp._name);
}
}