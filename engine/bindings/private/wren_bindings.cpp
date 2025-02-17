#include "wren_bindings.hpp"

#include "animation.hpp"
#include "application_module.hpp"
#include "audio/audio_bindings.hpp"
#include "audio_emitter_component.hpp"
#include "audio_module.hpp"
#include "cheats_component.hpp"
#include "components/name_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "game/game_bindings.hpp"
#include "input/input_bindings.hpp"
#include "particle_module.hpp"
#include "particles/particle_bindings.hpp"
#include "pathfinding/pathfinding_bindings.hpp"
#include "pathfinding_module.hpp"
#include "physics/physics_bindings.hpp"
#include "physics_module.hpp"
#include "renderer/animation_bindings.hpp"
#include "systems/lifetime_component.hpp"
#include "time_module.hpp"
#include "utility/math_bind.hpp"
#include "utility/wren_entity.hpp"
#include "wren_engine.hpp"

#include "game_module.hpp"
#include <cstdint>

namespace bindings
{

void BindEntity(wren::ForeignModule& module);

float TimeModuleGetDeltatime(TimeModule& self)
{
    return self.GetDeltatime().count();
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

uint32_t GetEntity(WrenEntity& self) { return static_cast<uint32_t>(self.entity); }
}

void BindEngineAPI(wren::ForeignModule& module)
{
    bindings::BindMath(module);
    bindings::BindEntity(module);

    // Add modules here to expose them in scripting
    {
        auto& engineAPI = module.klass<WrenEngine>("Engine");
        engineAPI.func<&WrenEngine::GetModule<TimeModule>>("GetTime");
        engineAPI.func<&WrenEngine::GetModule<ECSModule>>("GetECS");
        engineAPI.func<&WrenEngine::GetModule<ApplicationModule>>("GetInput");
        engineAPI.func<&WrenEngine::GetModule<AudioModule>>("GetAudio");
        engineAPI.func<&WrenEngine::GetModule<ParticleModule>>("GetParticles");
        engineAPI.func<&WrenEngine::GetModule<PhysicsModule>>("GetPhysics");
        engineAPI.func<&WrenEngine::GetModule<GameModule>>("GetGame");
        engineAPI.func<&WrenEngine::GetModule<PathfindingModule>>("GetPathfinding");
    }

    // Time Module
    {
        auto& time = module.klass<TimeModule>("TimeModule");
        time.funcExt<bindings::TimeModuleGetDeltatime>("GetDeltatime");
    }

    // ECS module
    {
        // ECS class
        auto& wrenClass = module.klass<ECSModule>("ECS");
        wrenClass.funcExt<bindings::CreateEntity>("NewEntity");
        wrenClass.funcExt<bindings::GetEntityByName>("GetEntityByName");
        wrenClass.funcExt<bindings::FreeEntity>("DestroyEntity");
    }

    // Input
    {
        BindInputAPI(module);
    }

    // Audio
    {
        BindAudioAPI(module);
    }

    // Animations
    {
        BindAnimationAPI(module);
    }

    // Particles
    {
        BindParticleAPI(module);
    }

    // Physics
    {
        BindPhysicsAPI(module);
    }

    // Pathfinding
    {
        BindPathfindingAPI(module);
    }

    // Game
    {
        BindGameAPI(module);
    }

    // Components
    {
        // Name class
        auto& nameClass = module.klass<WrenComponent<NameComponent>>("NameComponent");
        nameClass.propReadonlyExt<bindings::NameComponentGetName>("name");

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

void bindings::BindEntity(wren::ForeignModule& module)
{
    // Entity class
    auto& entityClass = module.klass<WrenEntity>("Entity");

    module.klass<entt::entity>("enttEntity");

    entityClass.funcExt<bindings::GetEntity>("GetEnttEntity");

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
}
