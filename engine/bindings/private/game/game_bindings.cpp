#include "game_bindings.hpp"

#include "cheats_component.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "entity/wren_entity.hpp"
#include "game_module.hpp"
#include "physics/shape_factory.hpp"
#include "physics_module.hpp"
#include "systems/lifetime_component.hpp"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

#include "ui_progress_bar.hpp"
#include "ui_text.hpp"

#include <ui_image.hpp>

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

void UpdateGrenadeBar(HUD& self, const float charge)
{
    if (auto locked = self.grenadeBar.lock(); locked != nullptr)
    {
        locked->SetFractionFilled(charge);
    }
}

void UpdateDashCharges(HUD& self, int charges)
{
    for (int32_t i = 0; i < static_cast<int32_t>(self.dashCharges.size()); i++)
    {
        if (auto locked = self.dashCharges[i].lock(); locked != nullptr)
        {
            if (i < charges) // Charge full
            {
                locked->display_color = glm::vec4(1);
            }
            else // Charge empty
            {
                locked->display_color = glm::vec4(1, 1, 1, 0.2);
            }
        }
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
    game.funcExt<bindings::AlterPlayerHeight>("AlterPlayerHeight");
    game.funcExt<bindings::GetHUD>("GetHUD");

    auto& hud = module.klass<HUD>("HUD");
    hud.funcExt<bindings::UpdateHealthBar>("UpdateHealthBar", "Update health bar with value from 0 to 1");
    hud.funcExt<bindings::UpdateAmmoText>("UpdateAmmoText", "Update ammo bar with a current ammo count and max");
    hud.funcExt<bindings::UpdateUltBar>("UpdateUltBar", "Update ult bar with value from 0 to 1");
    hud.funcExt<bindings::UpdateScoreText>("UpdateScoreText", "Update score text with score number");
    hud.funcExt<bindings::UpdateGrenadeBar>("UpdateGrenadeBar", "Update grenade bar with value from 0 to 1");
    hud.funcExt<bindings::UpdateDashCharges>("UpdateDashCharges", "Update dash bar with number of remaining charges");
}