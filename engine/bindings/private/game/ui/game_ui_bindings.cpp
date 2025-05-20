#include "game/ui/game_ui_bindings.hpp"
#include "game_module.hpp"
#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_progress_bar.hpp"
#include "ui_text.hpp"

namespace bindings
{

void ButtonOnPress(UIButton& self, wren::Variable fn) { self.OnPress(fn); }

std::shared_ptr<UIButton> PlayButton(MainMenu& self) { return self.playButton.lock(); }
std::shared_ptr<UIButton> QuitButton(MainMenu& self) { return self.quitButton.lock(); }
std::shared_ptr<UIButton> SettingsButton(MainMenu& self) { return self.settingsButton.lock(); }
std::shared_ptr<UIButton> MainControlsButton(MainMenu& self) { return self.controlsButton.lock(); }

std::shared_ptr<UIButton> ContinueButton(PauseMenu& self) { return self.continueButton.lock(); }
std::shared_ptr<UIButton> BackButton(PauseMenu& self) { return self.backToMainButton.lock(); }
std::shared_ptr<UIButton> PauseSettingsButton(PauseMenu& self) { return self.settingsButton.lock(); }
std::shared_ptr<UIButton> PauseControlsButton(PauseMenu& self) { return self.controlsButton.lock(); }

std::shared_ptr<UIButton> RetryButton(GameOverMenu& self) { return self.continueButton.lock(); }
std::shared_ptr<UIButton> GameOverMenuButton(GameOverMenu& self) { return self.backToMainButton.lock(); }

std::shared_ptr<UIButton> ControlsMenuBackButton(ControlsMenu& self) { return self.backButton.lock(); }

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

void UpdateMultiplierText(HUD& self, const float multiplier)
{
    if (auto locked = self.multiplierText.lock(); locked != nullptr)
    {
        locked->SetText(fmt::format("{:.1f}", multiplier).append("x"));
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

void UpdateUltReadyText(HUD& self, bool ready)
{
    if (auto locked = self.ultReadyText.lock(); locked != nullptr)
    {
        if (ready)
        {
            locked->SetText("ultimate ability is ready");
        }
        else
        {
            locked->SetText("");
        }
    }
}

std::shared_ptr<UIElement> AsBaseClass(std::shared_ptr<UIButton> self)
{
    return self;
}

void ShowHitmarker(HUD& self, bool val)
{
    auto hitmarker = self.hitmarker.lock();
    if (!hitmarker)
    {
        return;
    }

    hitmarker->visibility = val ? UIElement::VisibilityState::eUpdatedAndVisible : UIElement::VisibilityState::eNotUpdatedAndInvisible;
}

void ShowHitmarkerCrit(HUD& self, bool val)
{
    auto hitmarkerCrit = self.hitmarkerCrit.lock();
    if (!hitmarkerCrit)
    {
        return;
    }

    hitmarkerCrit->visibility = val ? UIElement::VisibilityState::eUpdatedAndVisible : UIElement::VisibilityState::eNotUpdatedAndInvisible;
}

void SetPowerupTextColor(HUD& self, glm::vec4 color)
{
    std::shared_ptr<UITextElement> powerupText = self.powerupText.lock();
    if (!powerupText)
    {
        return;
    }
    powerupText->SetColor(color);
}

void SetPowerupText(HUD& self, const std::string& text)
{
    std::shared_ptr<UITextElement> powerupText = self.powerupText.lock();
    if (!powerupText)
    {
        return;
    }
    powerupText->SetText(text);
}

glm::vec4 GetPowerupTextColor(HUD& self)
{
    std::shared_ptr<UITextElement> powerupText = self.powerupText.lock();
    if (powerupText)
    {
        // return;
        return powerupText->GetColor();
    }
}

}

void BindGameUI(wren::ForeignModule& module)
{
    auto& button = module.klass<UIButton, UIElement>("UIButton");
    button.funcExt<bindings::ButtonOnPress>("OnPress", "void callback() -> void");

    module.klass<Canvas, UIElement>("Canvas");

    auto& mainMenu = module.klass<MainMenu, Canvas>("MainMenu");
    mainMenu.propReadonlyExt<bindings::SettingsButton>("settingsButton");
    mainMenu.propReadonlyExt<bindings::MainControlsButton>("controlsButton");
    mainMenu.propReadonlyExt<bindings::QuitButton>("quitButton");
    mainMenu.propReadonlyExt<bindings::PlayButton>("playButton");

    auto& pauseMenu = module.klass<PauseMenu, Canvas>("PauseMenu");
    pauseMenu.propReadonlyExt<bindings::PauseSettingsButton>("settingsButton");
    pauseMenu.propReadonlyExt<bindings::BackButton>("backButton");
    pauseMenu.propReadonlyExt<bindings::ContinueButton>("continueButton");
    pauseMenu.propReadonlyExt<bindings::PauseControlsButton>("controlsButton");

    auto& hud = module.klass<HUD, Canvas>("HUD");

    hud.funcExt<bindings::UpdateHealthBar>("UpdateHealthBar", "Update health bar with value from 0 to 1");
    hud.funcExt<bindings::UpdateAmmoText>("UpdateAmmoText", "Update ammo bar with a current ammo count and max");
    hud.funcExt<bindings::UpdateUltBar>("UpdateUltBar", "Update ult bar with value from 0 to 1");
    hud.funcExt<bindings::UpdateScoreText>("UpdateScoreText", "Update score text with score number");
    hud.funcExt<bindings::UpdateGrenadeBar>("UpdateGrenadeBar", "Update grenade bar with value from 0 to 1");
    hud.funcExt<bindings::UpdateDashCharges>("UpdateDashCharges", "Update dash bar with number of remaining charges");
    hud.funcExt<bindings::UpdateMultiplierText>("UpdateMultiplierText", "Update multiplier number");
    hud.funcExt<bindings::UpdateUltReadyText>("UpdateUltReadyText", "Use bool to set if ultimate is ready");
    hud.funcExt<bindings::ShowHitmarker>("ShowHitmarker", "should show the hitmarker");
    hud.funcExt<bindings::ShowHitmarkerCrit>("ShowHitmarkerCrit", "should show the critical hitmarker");
    hud.funcExt<bindings::SetPowerupText>("SetPowerUpText", "Set powerup text");
    hud.funcExt<bindings::SetPowerupTextColor>("SetPowerUpTextColor", "Set powerup text color");
    hud.funcExt<bindings::GetPowerupTextColor>("GetPowerUpTextColor", "Get powerup text color");

    auto& gameOver
        = module.klass<GameOverMenu, Canvas>("GameOverMenu");

    gameOver.propReadonlyExt<bindings::GameOverMenuButton>("backButton");
    gameOver.propReadonlyExt<bindings::RetryButton>("retryButton");
}
