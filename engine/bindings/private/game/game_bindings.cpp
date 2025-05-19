#include "game_bindings.hpp"

#include "cheats_component.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "game_module.hpp"
#include "model_loading.hpp"
#include "physics/shape_factory.hpp"
#include "physics_module.hpp"
#include "systems/lifetime_component.hpp"
#include "ui/game_ui_bindings.hpp"
#include "ui_module.hpp"
#include "wren_entity.hpp"

#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

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

void SetGamepadActiveButton(UIModule& self, std::shared_ptr<UIElement> button)
{
    self.uiInputContext.focusedUIElement = button;
}

void MenuStackPush(GameModule& self, std::shared_ptr<Canvas> menu)
{
    self.PushUIMenu(menu);
}

void MenuStackPop(GameModule& self)
{
    self.PopUIMenu();
}

void MenuStackSet(GameModule& self, std::shared_ptr<Canvas> menu)
{
    self.SetUIMenu(menu);
}

GameSettings* GetSettings(GameModule& self)
{
    return &self.GetSettings();
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

    auto& settings = module.klass<GameSettings>("GameSettings");
    settings.var<&GameSettings::aimSensitivity>("Sensitivity");
    settings.var<&GameSettings::aimSensitivity>("aimSensitivity");

    game.func<&GameModule::GetMainMenu>("GetMainMenu");
    game.func<&GameModule::GetPauseMenu>("GetPauseMenu");
    game.func<&GameModule::GetHUD>("GetHUD");
    game.func<&GameModule::GetGameOver>("GetGameOverMenu");

    game.func<&GameModule::GetAimAssistDirection>("GetAimAssistDirection", "Returns the direction vector where to shoot to according to the aim assist");

    game.funcExt<&bindings::GetSettings>("GetSettings");
    game.funcExt<&bindings::MenuStackSet>("SetUIMenu");
    game.funcExt<&bindings::MenuStackPush>("PushUIMenu");
    game.funcExt<&bindings::MenuStackPop>("PopUIMenu");

    auto& ui = module.klass<UIModule>("UIModule");
    module.klass<UIElement>("UIElement");

    ui.funcExt<bindings::SetGamepadActiveButton>("SetSelectedElement");

    BindGameUI(module);
}
