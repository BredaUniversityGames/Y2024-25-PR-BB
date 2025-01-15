#include "game_bindings.hpp"

#include "lifetime_component.hpp"
#include "physics_module.hpp"
#include "utility/wren_entity.hpp"
#include "wren_game.hpp"

#include "ecs_module.hpp"

#include "components/name_component.hpp"
#include "components/rigidbody_component.hpp"

namespace bindings
{
void SetLifetimePaused(WrenComponent<LifetimeComponent>& self, bool paused)
{
    self.component->paused = paused;
}

bool GetLifetimePaused(WrenComponent<LifetimeComponent>& self)
{
    return self.component->paused;
}

void SetLifetime(WrenComponent<LifetimeComponent>& self, float lifetime)
{
    self.component->lifetime = lifetime;
}

float GetLifetime(WrenComponent<LifetimeComponent>& self)
{
    return self.component->lifetime;
}

WrenEntity CreatePlayer(WrenGame& self, PhysicsModule& physicsModule, ECSModule& ecs, const glm::vec3& position, const float height, const float radius)
{
    entt::entity playerEntity = ecs.GetRegistry().create();
    JPH::BodyCreationSettings bodyCreationSettings(new JPH::CapsuleShape(height, radius), JPH::Vec3(position.x, position.y, position.z), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
    bodyCreationSettings.mAllowDynamicOrKinematic = true;

    bodyCreationSettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    RigidbodyComponent rb(*physicsModule.bodyInterface, playerEntity, bodyCreationSettings);

    NameComponent node;
    node.name = "Player entity";
    ecs.GetRegistry().emplace<NameComponent>(playerEntity, node);
    ecs.GetRegistry().emplace<RigidbodyComponent>(playerEntity, rb);
    return { playerEntity, &ecs.GetRegistry() };
}
}

void BindGameAPI(wren::ForeignModule& module)
{
    auto& lifetimeComponent = module.klass<WrenComponent<LifetimeComponent>>("LifetimeComponent");
    lifetimeComponent.propExt<bindings::GetLifetimePaused, bindings::SetLifetimePaused>("paused");
    lifetimeComponent.propExt<bindings::GetLifetime, bindings::SetLifetime>("lifetime");

    auto& game = module.klass<WrenGame>("Game");
    game.funcExt<bindings::CreatePlayer>("CreatePlayer");
}