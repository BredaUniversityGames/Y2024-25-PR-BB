#include "ecs_module.hpp"
#include "components/relationship_helpers.hpp"
#include "components/transform_helpers.hpp"
#include "scripting_module.hpp"
#include "systems/physics_system.hpp"
#include "time_module.hpp"
#include "utility/wren_entity.hpp"

#include <components/name_component.hpp>

namespace bindings
{

WrenEntity CreateEntity(ECSModule& self)
{
    return { self.GetRegistry().create(), &self.GetRegistry() };
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

void TransformComponentSetTranslation(WrenComponent<TransformComponent>& component, const glm::vec3& translation)
{
    TransformHelpers::SetLocalPosition(*component.entity.registry, component.entity.entity, translation);
}

std::string NameComponentGetName(WrenComponent<NameComponent>& nameComponent)
{
    return nameComponent.component->name;
}

}

ModuleTickOrder ECSModule::Init(MAYBE_UNUSED Engine& engine)
{
    TransformHelpers::SubscribeToEvents(registry);
    RelationshipHelpers::SubscribeToEvents(registry);

    // ECS class
    auto& scripting = engine.GetModule<ScriptingModule>();
    auto& wren_class = scripting.GetForeignAPI().klass<ECSModule>("ECS");
    wren_class.funcExt<bindings::CreateEntity>("NewEntity");
    wren_class.funcExt<bindings::GetEntityByName>("GetEntityByName");

    // Entity class
    auto& entityClass = scripting.GetForeignAPI().klass<WrenEntity>("Entity");
    entityClass.func<&WrenEntity::GetComponent<TransformComponent>>("GetTransformComponent");
    entityClass.func<&WrenEntity::AddComponent<TransformComponent>>("AddTransformComponent");

    // Name class
    auto& nameClass = scripting.GetForeignAPI().klass<WrenComponent<NameComponent>>("NameComponent");
    nameClass.propReadonlyExt<bindings::NameComponentGetName>("name");

    // Transform component
    auto& transformClass = scripting.GetForeignAPI().klass<WrenComponent<TransformComponent>>("TransformComponent");
    transformClass.funcExt<bindings::TransformComponentSetTranslation>("SetTranslation");

    return ModuleTickOrder::eTick;
}

void ECSModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    TransformHelpers::UnsubscribeToEvents(registry);
    RelationshipHelpers::UnsubscribeToEvents(registry);
}

void ECSModule::Tick(Engine& engine)
{
    auto dt = engine.GetModule<TimeModule>().GetDeltatime().count();

    RemovedDestroyed();
    UpdateSystems(dt);
    RenderSystems();
}

void ECSModule::UpdateSystems(const float dt)
{
    for (auto& system : systems)
    {
        system->Update(*this, dt);
    }
}
void ECSModule::RenderSystems() const
{
    for (const auto& system : systems)
    {
        system->Render(*this);
    }
}
void ECSModule::RemovedDestroyed()
{
    // TODO: should be somewhere else
    if (auto* physics = GetSystem<PhysicsSystem>())
    {
        physics->CleanUp();
    }

    const auto toDestroy = registry.view<DeleteTag>();
    for (const entt::entity entity : toDestroy)
    {
        registry.destroy(entity);
    }
}

void ECSModule::DestroyEntity(entt::entity entity)
{
    assert(registry.valid(entity));
    registry.emplace_or_replace<DeleteTag>(entity);
}