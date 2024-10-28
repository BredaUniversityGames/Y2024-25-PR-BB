#include "components/name_component.hpp"

#include <entt/entity/registry.hpp>

std::string_view NameComponent::GetDisplayName(const entt::registry& registry, entt::entity entity)
{
    if (auto* ptr = registry.try_get<NameComponent>(entity))
    {
        return std::string_view { ptr->name };
    }

    return std::string_view { "Unnamed Entity" };
}