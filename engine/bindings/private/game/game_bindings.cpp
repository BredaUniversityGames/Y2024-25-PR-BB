#include "game_bindings.hpp"

#include "cheats_component.hpp"
#include "components/name_component.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "entity/wren_entity.hpp"
#include "game_module.hpp"
#include "physics/shape_factory.hpp"
#include "physics_module.hpp"
#include "systems/lifetime_component.hpp"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

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
    ecs.GetRegistry().emplace<TransformComponent>(playerEntity);
    TransformHelpers::SetLocalPosition(ecs.GetRegistry(), playerEntity, position);

    auto degreesOfFreedom = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    RigidbodyComponent rb(physicsModule.GetBodyInterface(), ShapeFactory::MakeCapsuleShape(height, radius), true, degreesOfFreedom);

    NameComponent node;
    PlayerTag playerTag;
    node.name = "Player entity";
    ecs.GetRegistry().emplace<NameComponent>(playerEntity, node);
    ecs.GetRegistry().emplace<RigidbodyComponent>(playerEntity, rb);
    ecs.GetRegistry().emplace<PlayerTag>(playerEntity, playerTag);
    return { playerEntity, &ecs.GetRegistry() };
}

// Do not pass heights smaller than 0.1f, it will get clamped for saftey to 0.1f
void AlterPlayerHeight(MAYBE_UNUSED GameModule& self, PhysicsModule& physicsModule, ECSModule& ecs, const float height)
{
    auto playerView = ecs.GetRegistry().view<PlayerTag>();
    for (auto entity : playerView)
    {
        auto& rb = ecs.GetRegistry().get<RigidbodyComponent>(entity);
        const auto& shape = physicsModule.GetBodyInterface().GetShape(rb.bodyID);
        auto capsuleShape = JPH::StaticCast<JPH::CapsuleShape>(shape);
        if (capsuleShape == nullptr)
        {
            return;
        }
        const float radius = capsuleShape->GetRadius();
        physicsModule.GetBodyInterface().SetShape(rb.bodyID, new JPH::CapsuleShape(height / 2.0, radius), true, JPH::EActivation::Activate);
    }
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
    game.funcExt<bindings::AlterPlayerHeight>("AlterPlayerHeight");
}