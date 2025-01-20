#include "game_bindings.hpp"

#include "cheats_component.hpp"
#include "lifetime_component.hpp"
#include "physics_module.hpp"
#include "utility/wren_entity.hpp"

#include "ecs_module.hpp"

#include "components/name_component.hpp"
#include "components/rigidbody_component.hpp"

#include <game_module.hpp>

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

bool GetNoClipStatus(WrenComponent<CheatsComponent>& self)
{
    return self.component->noClip;
}

void SetNoClip(WrenComponent<CheatsComponent>& self, bool noClip)
{
    self.component->noClip = noClip;
}

WrenEntity CreatePlayerController(MAYBE_UNUSED GameModule& self, PhysicsModule& physicsModule, ECSModule& ecs, const glm::vec3& position, const float height, const float radius)
{

    auto playerView = ecs.GetRegistry().view<PlayerTag>();
    for (auto entity : playerView)
    {
        ecs.DestroyEntity(entity);
    }
    entt::entity playerEntity = ecs.GetRegistry().create();
    JPH::BodyCreationSettings bodyCreationSettings(new JPH::CapsuleShape(height, radius), JPH::Vec3(position.x, position.y, position.z), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, PhysicsLayers::MOVING);
    bodyCreationSettings.mAllowDynamicOrKinematic = true;

    bodyCreationSettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    RigidbodyComponent rb(*physicsModule.bodyInterface, playerEntity, bodyCreationSettings);

    NameComponent node;
    PlayerTag playerTag;
    node.name = "Player entity";
    ecs.GetRegistry().emplace<NameComponent>(playerEntity, node);
    ecs.GetRegistry().emplace<RigidbodyComponent>(playerEntity, rb);
    ecs.GetRegistry().emplace<PlayerTag>(playerEntity, playerTag);
    return { playerEntity, &ecs.GetRegistry() };
}
}

void BindGameAPI(wren::ForeignModule& module)
{
    auto& lifetimeComponent = module.klass<WrenComponent<LifetimeComponent>>("LifetimeComponent");
    lifetimeComponent.propExt<bindings::GetLifetimePaused, bindings::SetLifetimePaused>("paused");
    lifetimeComponent.propExt<bindings::GetLifetime, bindings::SetLifetime>("lifetime");

    auto& cheatsComponent = module.klass<WrenComponent<CheatsComponent>>("CheatsComponent");
    cheatsComponent.propExt<bindings::GetNoClipStatus, bindings::SetNoClip>("noClip");

    auto& game = module.klass<GameModule>("Game");
    game.funcExt<bindings::CreatePlayerController>("CreatePlayerController");
}