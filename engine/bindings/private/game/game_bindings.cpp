#include "game_bindings.hpp"

#include "cheats_component.hpp"
#include "components/name_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "entity/wren_entity.hpp"
#include "game_module.hpp"
#include "physics/shape_factory.hpp"
#include "physics_module.hpp"
#include "systems/lifetime_component.hpp"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

#include "components/relationship_component.hpp"
#include "components/relationship_helpers.hpp"

#include "components/camera_component.hpp"

#include "ui_progress_bar.hpp"
#include "ui_text.hpp"

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

    auto playerView = ecs.GetRegistry().view<NameComponent>();
    for (auto entity : playerView)
    {
        auto& name = ecs.GetRegistry().get<NameComponent>(entity);
        if (name.name == "Player")
        {
            ecs.DestroyEntity(entity);
        }
        if (name.name == "Player entity")
        {
            ecs.DestroyEntity(entity);
        }
        if (name.name == "Camera")
        {
            ecs.DestroyEntity(entity);
        }
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
    ecs.GetRegistry().emplace<RelationshipComponent>(playerEntity);

    entt::entity entity = ecs.GetRegistry().create();
    ecs.GetRegistry().emplace<NameComponent>(entity, "Player");
    ecs.GetRegistry().emplace<TransformComponent>(entity);
    ecs.GetRegistry().emplace<RelationshipComponent>(entity);

    entt::entity cameraEntity = ecs.GetRegistry().create();
    ecs.GetRegistry().emplace<NameComponent>(cameraEntity, "Camera");
    ecs.GetRegistry().emplace<TransformComponent>(cameraEntity);
    ecs.GetRegistry().emplace<RelationshipComponent>(cameraEntity);

    RelationshipHelpers::AttachChild(ecs.GetRegistry(), entity, cameraEntity);

    auto view = ecs.GetRegistry().view<NameComponent>();
    for (auto&& [e, n] : view.each())
    {
        if (n.name == "AnimatedRifle")
        {
            RelationshipHelpers::AttachChild(ecs.GetRegistry(), cameraEntity, e);
            break;
        }
    }

    CameraComponent& cameraComponent
        = ecs.GetRegistry().emplace<CameraComponent>(cameraEntity);
    cameraComponent.projection = CameraComponent::Projection::ePerspective;
    cameraComponent.fov = 45.0f;
    cameraComponent.nearPlane = 0.5f;
    cameraComponent.farPlane = 600.0f;
    cameraComponent.reversedZ = true;

    ecs.GetRegistry().emplace<AudioListenerComponent>(cameraEntity);

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

HUD& GetHUD(GameModule& self)
{
    return self._hud;
}

void UpdateHealthBar(HUD& self, const float health)
{
    if (auto locked = self.healthBar.lock(); locked != nullptr)
    {
        locked->SetFractionFilled(health);
    }
}

void UpdateAmmoText(HUD& self, const int ammo, const int maxAmmo)
{
    if (auto locked = self.ammoCounter.lock(); locked != nullptr)
    {
        locked->SetText(std::to_string(ammo) + "/" + std::to_string(maxAmmo));
    }
}

void UpdateUltBar(HUD& self, const float ult)
{
    if (auto locked = self.ultBar.lock(); locked != nullptr)
    {
        locked->SetFractionFilled(ult);
    }
}

void UpdateScoreText(HUD& self, const int score)
{
    if (auto locked = self.scoreText.lock(); locked != nullptr)
    {
        locked->SetText(std::string("Score: ") + std::to_string(score));
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
    game.funcExt<bindings::GetHUD>("GetHUD");

    auto& hud = module.klass<HUD>("HUD");
    hud.funcExt<bindings::UpdateHealthBar>("UpdateHealthBar");
    hud.funcExt<bindings::UpdateAmmoText>("UpdateAmmoText");
    hud.funcExt<bindings::UpdateUltBar>("UpdateUltBar");
    hud.funcExt<bindings::UpdateScoreText>("UpdateScoreText");
}