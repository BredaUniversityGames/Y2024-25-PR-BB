#include "ecs_module.hpp"
#include "wren_common.hpp"
#include "wren_entity.hpp"

#include "animation.hpp"
#include "audio_emitter_component.hpp"
#include "cheats_component.hpp"
#include "components/camera_component.hpp"
#include "components/name_component.hpp"
#include "components/point_light_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
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

std::string NameComponentGetName(WrenComponent<NameComponent>& nameComponent)
{
    return nameComponent.component->name;
}

void PointLightComponentSetColor(WrenComponent<PointLightComponent>& component, const glm::vec3& color)
{
    component.component->color = color;
}

glm::vec3 PointLightComponentGetColor(WrenComponent<PointLightComponent>& component)
{
    return component.component->color;
}

float CameraGetFOV(WrenComponent<CameraComponent>& component)
{
    return component.component->fov;
}

void CameraSetFOV(WrenComponent<CameraComponent>& component, const float fov)
{
    component.component->fov = fov;
}

uint32_t GetEntity(WrenEntity& self) { return static_cast<uint32_t>(self.entity); }

void BindEntity(wren::ForeignModule& module)
{
    // Entity class
    auto& entityClass = module.klass<WrenEntity>("Entity");
    entityClass.funcExt<GetEntity>("GetEnttEntity");

    entityClass.func<&WrenEntity::GetComponent<TransformComponent>>("GetTransformComponent");
    entityClass.func<&WrenEntity::AddComponent<TransformComponent>>("AddTransformComponent");

    entityClass.func<&WrenEntity::GetComponent<AudioEmitterComponent>>("GetAudioEmitterComponent");
    entityClass.func<&WrenEntity::AddComponent<AudioEmitterComponent>>("AddAudioEmitterComponent");

    entityClass.func<&WrenEntity::GetComponent<NameComponent>>("GetNameComponent");
    entityClass.func<&WrenEntity::AddComponent<NameComponent>>("AddNameComponent");

    entityClass.func<&WrenEntity::GetComponent<LifetimeComponent>>("GetLifetimeComponent");
    entityClass.func<&WrenEntity::AddComponent<LifetimeComponent>>("AddLifetimeComponent");

    entityClass.func<&WrenEntity::GetComponent<CheatsComponent>>("GetCheatsComponent");
    entityClass.func<&WrenEntity::AddComponent<CheatsComponent>>("AddCheatsComponent");

    entityClass.func<&WrenEntity::GetComponent<AnimationControlComponent>>("GetAnimationControlComponent");

    entityClass.func<&WrenEntity::GetComponent<RigidbodyComponent>>("GetRigidbodyComponent");

    entityClass.func<&WrenEntity::GetComponent<PointLightComponent>>("GetPointLightComponent");
    entityClass.func<&WrenEntity::AddComponent<PointLightComponent>>("AddPointLightComponent");

    entityClass.func<&WrenEntity::GetComponent<CameraComponent>>("GetCameraComponent");
}

WrenEntity CreateEntity(ECSModule& self)
{
    return { self.GetRegistry().create(), &self.GetRegistry() };
}

void FreeEntity(ECSModule& self, WrenEntity& entity)
{
    self.GetRegistry().destroy(entity.entity);
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
        nameClass.propReadonlyExt<bindings::NameComponentGetName>("name");

        auto& pointLightClass = module.klass<WrenComponent<PointLightComponent>>("PointLightComponent");
        pointLightClass.propExt<
            bindings::PointLightComponentGetColor, bindings::PointLightComponentSetColor>("color");

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

        auto& cameraClass = module.klass<WrenComponent<CameraComponent>>("CameraComponent");
        cameraClass.propExt<bindings::CameraGetFOV, bindings::CameraSetFOV>("fov");
    }
}