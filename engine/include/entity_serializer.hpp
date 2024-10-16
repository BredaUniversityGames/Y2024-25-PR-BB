#pragma once
#include "include_cereal.hpp"

#include <entt/entity/registry.hpp>

class EntitySerializer
{
public:
    EntitySerializer(const entt::registry& registry, entt::entity entity = entt::null)
        : _registry(registry)
        , _entity(entity)
    {
    }   
    
    template <class Archive>
    void save(Archive& archive, uint32_t const version) const;

private:
    const entt::registry& _registry;
    entt::entity _entity;
};

CEREAL_CLASS_VERSION(EntitySerializer, 0);
template <class Archive>
void EntitySerializer::save(Archive& archive, uint32_t version) const
{
    static auto trySaveComponent = [&]<typename T>()
    {
        if (auto component = _registry.try_get<T>(_entity); component != nullptr)
            archive(cereal::make_nvp(typeid(T).name(), *component));
    };

    if (version == 0)
    {
        //add components here
    }
}
