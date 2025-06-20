#include "game_bindings.hpp"

#include "aim_assist.hpp"
#include "cheats_component.hpp"
#include "components/rigidbody_component.hpp"
#include "ecs_module.hpp"
#include "game_module.hpp"
#include "physics_module.hpp"
#include "systems/lifetime_component.hpp"
#include "ui/game_ui_bindings.hpp"
#include "ui_module.hpp"
#include "wren_entity.hpp"
#include "wren_tracy_zone.hpp"


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

glm::vec3 GetFlashColor(GameModule& self)
{
    return glm::vec3(self.GetFlashColor().x, self.GetFlashColor().y, self.GetFlashColor().z);
}

float GetFlashIntensity(GameModule& self)
{
    return self.GetFlashColor().w;
}

void SetFlashColor(GameModule& self, const glm::vec3& color, const float intensity)
{
    self.GetFlashColor() = glm::vec4(color.r, color.g, color.b, intensity);
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

glm::vec3 GetAimAssistDirection(GameModule&, ECSModule& ecs, PhysicsModule& physics, const glm::vec3& position, const glm::vec3& forward, float range, float minAngle)
{
    return AimAssist::GetAimAssistDirection(ecs, physics, position, forward, range, minAngle);
}

std::vector<CachedBindingOriginVisual> GetDigitalActionBindingOriginVisual(GameModule& self, const std::string& actionName)
{
    return self.GetInputVisualiztionsCache().GetDigital(actionName);
}

std::vector<CachedBindingOriginVisual> GetAnalogActionBindingOriginVisual(GameModule& self, const std::string& actionName)
{
    return self.GetInputVisualiztionsCache().GetAnalog(actionName);
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
    game.funcExt<&bindings::GetFlashColor>("GetFlashColor");
    game.funcExt<&bindings::GetFlashIntensity>("GetFlashIntensity");
    game.funcExt<&bindings::SetFlashColor>("SetFlashColor");

    auto& settings = module.klass<GameSettings>("GameSettings");
    settings.var<&GameSettings::aimSensitivity>("Sensitivity");
    settings.var<&GameSettings::aimSensitivity>("aimSensitivity");
    settings.var<&GameSettings::aimAssist>("aimAssist");
    settings.var<&GameSettings::fov>("fov");

    game.func<&GameModule::GetMainMenu>("GetMainMenu");
    game.func<&GameModule::GetPauseMenu>("GetPauseMenu");
    game.func<&GameModule::GetHUD>("GetHUD");
    game.func<&GameModule::GetGameOver>("GetGameOverMenu");
    game.func<&GameModule::GetLoadingScreen>("GetLoadingScreen");
    game.func<&GameModule::GetGameVersionVisual>("GetGameVersionVisual");

    game.funcExt<&bindings::GetAimAssistDirection>("GetAimAssistDirection", "Returns the direction vector where to shoot to according to the aim assist");

    game.funcExt<&bindings::GetSettings>("GetSettings");
    game.funcExt<&bindings::MenuStackSet>("SetUIMenu");
    game.funcExt<&bindings::MenuStackPush>("PushUIMenu");
    game.funcExt<&bindings::MenuStackPop>("PopUIMenu");

    auto& bindingOriginVisual = module.klass<CachedBindingOriginVisual>("BindingOriginVisual");
    bindingOriginVisual.varReadonly<&CachedBindingOriginVisual::bindingInputName>("bindingInputName");

    game.funcExt<&bindings::GetDigitalActionBindingOriginVisual>("GetDigitalActionBindingOriginVisual");
    game.funcExt<&bindings::GetAnalogActionBindingOriginVisual>("GetAnalogActionBindingOriginVisual");

    auto& ui = module.klass<UIModule>("UIModule");
    module.klass<UIElement>("UIElement");

    ui.funcExt<bindings::SetGamepadActiveButton>("SetSelectedElement");

    auto& tracyZone = module.klass<WrenTracyZone>("TracyZone");

    tracyZone.ctor<const std::string&>();
    tracyZone.func<&WrenTracyZone::End>("End");

    BindGameUI(module);
}
