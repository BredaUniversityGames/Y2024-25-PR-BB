#include "components/name_component.hpp"

#include <entity/registry.hpp>

std::string NameComponent::GetDisplayName(const entt::registry& registry, entt::entity entity)
{
    const NameComponent* nameComponent = registry.try_get<NameComponent>(entity);
    return nameComponent == nullptr ? std::string("Entity-") + std::to_string(static_cast<uint32_t>(entity)) : nameComponent->_name;
}