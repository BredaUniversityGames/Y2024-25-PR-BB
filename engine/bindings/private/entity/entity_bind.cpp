#include "ecs_module.hpp"
#include "wren_common.hpp"
#include "wren_entity.hpp"

#include "animation.hpp"
#include "audio_emitter_component.hpp"
#include "cheats_component.hpp"
#include "components/directional_light_component.hpp"
#include "components/name_component.hpp"
#include "components/point_light_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "game_module.hpp"
#include "systems/lifetime_component.hpp"

namespace bindings
{

glm::vec3 TransformComponentGetTranslation(WrenComponent<TransformComponent>& component)
{
    return component.component->GetLocalPosition();
}

glm::quat TransformComponentGetRotation(WrenComponent<TransformComponent>& component)
{
    return component.component->GetLocalRotation();
}

glm::vec3 TransformComponentGetScale(WrenComponent<TransformComponent>& component)
{
    return component.component->GetLocalScale();
}

glm::vec3 TransformHelpersGetWorldTranslation(WrenComponent<TransformComponent>& component)
{
    return TransformHelpers::GetWorldPosition(*component.entity.registry, component.entity.entity);
}

glm::quat TransformHelpersGetWorldRotation(WrenComponent<TransformComponent>& component)
{
    return TransformHelpers::GetWorldRotation(*component.entity.registry, component.entity.entity);
}

glm::vec3 TransformHelpersGetWorldScale(WrenComponent<TransformComponent>& component)
{
    return TransformHelpers::GetWorldScale(*component.entity.registry, component.entity.entity);
}

void TransformHelpersSetWorldTransform(WrenComponent<TransformComponent>& component, glm::vec3 translation, glm::quat rotation, glm::vec3 scale)
{
    TransformHelpers::SetWorldTransform(*component.entity.registry, component.entity.entity, translation, rotation, scale);
}

void TransformComponentSetTranslation(WrenComponent<TransformComponent>& component, const glm::vec3& translation)
{
    TransformHelpers::SetLocalPosition(*component.entity.registry, component.entity.entity, translation);
}

void TransformComponentSetRotation(WrenComponent<TransformComponent>& component, const glm::quat& rotation)
{
    TransformHelpers::SetLocalRotation(*component.entity.registry, component.entity.entity, rotation);
}

void TransformComponentSetScale(WrenComponent<TransformComponent>& component, const glm::vec3& scale)
{
    TransformHelpers::SetLocalScale(*component.entity.registry, component.entity.entity, scale);
}

void NameComponentSetName(WrenComponent<NameComponent>& nameComponent, const std::string& name)
{
    nameComponent.component->name = name;
}

std::string NameComponentGetName(WrenComponent<NameComponent>& nameComponent)
{
    return nameComponent.component->name;
}

void NameComponentSetName(WrenComponent<NameComponent>& nameComponent, const std::string& name)
{
    nameComponent.component->name = name;
}

void PointLightComponentSetColor(WrenComponent<PointLightComponent>& component, const glm::vec3& color)
{
    component.component->color = color;
}

glm::vec3 PointLightComponentGetColor(WrenComponent<PointLightComponent>& component)
{
    return component.component->color;
}

void DirectionalLightComponentSetColor(WrenComponent<DirectionalLightComponent>& component, const glm::vec3& color)
{
    component.component->color = color;
}

glm::vec3 DirectionalLightComponentGetColor(WrenComponent<DirectionalLightComponent>& component)
{
    return component.component->color;
}

void DirectionalLightComponentSetViewPlanes(WrenComponent<DirectionalLightComponent>& component, const glm::vec2& planes)
{
    component.component->nearPlane = planes.x;
    component.component->farPlane = planes.y;
}

glm::vec2 DirectionalLightComponentGetViewPlanes(WrenComponent<DirectionalLightComponent>& component)
{
    return { component.component->nearPlane, component.component->farPlane };
}

void DirectionalLightComponentSetOrthographicSize(WrenComponent<DirectionalLightComponent>& component, float orthographicSize)
{
    component.component->orthographicSize = orthographicSize;
}

float DirectionalLightComponentGetOrthographicSize(WrenComponent<DirectionalLightComponent>& component)
{
    return component.component->orthographicSize;
}

uint32_t GetEntity(WrenEntity& self) { return static_cast<uint32_t>(self.entity); }

void BindEntity(wren::ForeignModule& module)
{
    // Entity class
    auto& entityClass = module.klass<WrenEntity>("Entity");
    entityClass.funcExt<GetEntity>("GetEnttEntity");

    entityClass.func<&WrenEntity::AddTag<PlayerTag>>("AddPlayerTag");

    entityClass.func<&WrenEntity::GetComponent<TransformComponent>>("GetTransformComponent");
    entityClass.func<&WrenEntity::AddDefaultComponent<TransformComponent>>("AddTransformComponent");

    entityClass.func<&WrenEntity::GetComponent<AudioEmitterComponent>>("GetAudioEmitterComponent");
    entityClass.func<&WrenEntity::AddDefaultComponent<AudioEmitterComponent>>("AddAudioEmitterComponent");

    entityClass.func<&WrenEntity::GetComponent<NameComponent>>("GetNameComponent");
    entityClass.func<&WrenEntity::AddDefaultComponent<NameComponent>>("AddNameComponent");

    entityClass.func<&WrenEntity::GetComponent<LifetimeComponent>>("GetLifetimeComponent");
    entityClass.func<&WrenEntity::AddDefaultComponent<LifetimeComponent>>("AddLifetimeComponent");

    entityClass.func<&WrenEntity::GetComponent<CheatsComponent>>("GetCheatsComponent");
    entityClass.func<&WrenEntity::AddDefaultComponent<CheatsComponent>>("AddCheatsComponent");

    entityClass.func<&WrenEntity::GetComponent<AnimationControlComponent>>("GetAnimationControlComponent");

    entityClass.func<&WrenEntity::GetComponent<RigidbodyComponent>>("GetRigidbodyComponent");
    entityClass.func<&WrenEntity::AddComponent<RigidbodyComponent>>("AddRigidbodyComponent");

    entityClass.func<&WrenEntity::GetComponent<PointLightComponent>>("GetPointLightComponent");
    entityClass.func<&WrenEntity::AddDefaultComponent<PointLightComponent>>("AddPointLightComponent");

    entityClass.func<&WrenEntity::GetComponent<DirectionalLightComponent>>("GetDirectionalLightComponent");
    entityClass.func<&WrenEntity::AddDefaultComponent<DirectionalLightComponent>>("AddDirectionalLightComponent");
}

WrenEntity CreateEntity(ECSModule& self)
{
    return { self.GetRegistry().create(), &self.GetRegistry() };
}

void FreeEntity(ECSModule& self, WrenEntity& entity)
{
    if (!self.GetRegistry().valid(entity.entity))
        return;
    self.DestroyEntity(entity.entity);
}

std::optional<WrenEntity> GetEntityByName(ECSModule& self, const std::string& name)
{
    auto view = self.GetRegistry().view<NameComponent>();
    for (auto&& [e, n] : view.each())
    {
        if (n.name == name)
        {
            return WrenEntity { e, &self.GetRegistry() };
        }
    }

    return std::nullopt;
}

std::vector<WrenEntity> GetEntitiesByName(ECSModule& self, const std::string& name)
{
    std::vector<WrenEntity> entities {};
    auto view = self.GetRegistry().view<NameComponent>();
    for (auto&& [e, n] : view.each())
    {
        if (n.name == name)
        {
            entities.emplace_back(WrenEntity { e, &self.GetRegistry() });
        }
    }

    return entities;
}

}

void BindEntityAPI(wren::ForeignModule& module)
{

    bindings::BindEntity(module);

    // ECS module
    {
        // ECS class
        auto& wrenClass = module.klass<ECSModule>("ECS");
        wrenClass.funcExt<bindings::CreateEntity>("NewEntity");
        wrenClass.funcExt<bindings::GetEntityByName>("GetEntityByName");
        wrenClass.funcExt<bindings::GetEntitiesByName>("GetEntitiesByName");
        wrenClass.funcExt<bindings::FreeEntity>("DestroyEntity");
    }
    // Components
    {
        // Name class
        auto& nameClass = module.klass<WrenComponent<NameComponent>>("NameComponent");
        nameClass.propExt<bindings::NameComponentGetName, bindings::NameComponentSetName>("name");

        auto& pointLightClass = module.klass<WrenComponent<PointLightComponent>>("PointLightComponent");
        pointLightClass.propExt<bindings::PointLightComponentGetColor, bindings::PointLightComponentSetColor>("color");

        auto& directionalLightClass = module.klass<WrenComponent<DirectionalLightComponent>>("DirectionalLightComponent");
        directionalLightClass.propExt<bindings::DirectionalLightComponentGetColor, bindings::DirectionalLightComponentSetColor>("color");
        directionalLightClass.propExt<bindings::DirectionalLightComponentGetViewPlanes, bindings::DirectionalLightComponentSetViewPlanes>("planes");
        directionalLightClass.propExt<bindings::DirectionalLightComponentGetOrthographicSize, bindings::DirectionalLightComponentSetOrthographicSize>("orthographicSize");

        // Transform component
        auto& transformClass = module.klass<WrenComponent<TransformComponent>>("TransformComponent");

        transformClass.propExt<
            bindings::TransformComponentGetTranslation, bindings::TransformComponentSetTranslation>("translation");

        transformClass.propExt<
            bindings::TransformComponentGetRotation, bindings::TransformComponentSetRotation>("rotation");

        transformClass.propExt<
            bindings::TransformComponentGetScale, bindings::TransformComponentSetScale>("scale");

        transformClass.funcExt<bindings::TransformHelpersGetWorldTranslation>("GetWorldTranslation");
        transformClass.funcExt<bindings::TransformHelpersGetWorldRotation>("GetWorldRotation");
        transformClass.funcExt<bindings::TransformHelpersGetWorldScale>("GetWorldScale");

        transformClass.funcExt<bindings::TransformHelpersSetWorldTransform>("SetWorldTransform");
    }
}