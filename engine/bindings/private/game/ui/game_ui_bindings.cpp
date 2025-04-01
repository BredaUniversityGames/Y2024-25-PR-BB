#include "game/ui/game_ui_bindings.hpp"
#include "ui/ui_menus.hpp"

namespace bindings
{

void ButtonOnPress(UIButton& self, wren::Variable fn) { self.OnPress(fn); }

std::shared_ptr<UIButton> PlayButton(MainMenu& self) { return self.playButton.lock(); }
std::shared_ptr<UIButton> QuitButton(MainMenu& self) { return self.quitButton.lock(); }
std::shared_ptr<UIButton> SettingsButton(MainMenu& self) { return self.settingsButton.lock(); }

std::shared_ptr<UIButton> ContinueButton(PauseMenu& self) { return self.continueButton.lock(); }
std::shared_ptr<UIButton> BackButton(PauseMenu& self) { return self.backToMainButton.lock(); }
std::shared_ptr<UIButton> PauseSettingsButton(PauseMenu& self) { return self.settingsButton.lock(); }

}

void BindMainMenu(wren::ForeignModule& module)
{
    auto& cls = module.klass<Callback>("Callback");
    cls.ctor<wren::Variable>();

    auto& button = module.klass<UIButton>("UIButton");
    button.funcExt<bindings::ButtonOnPress>("OnPress", "Pass a wren function to define the logic that is called OnPress");

    auto& mainMenu = module.klass<MainMenu>("MainMenu");
    mainMenu.propReadonlyExt<bindings::SettingsButton>("settingsButton");
    mainMenu.propReadonlyExt<bindings::QuitButton>("quitButton");
    mainMenu.propReadonlyExt<bindings::PlayButton>("playButton");

    auto& pauseMenu = module.klass<PauseMenu>("PauseMenu");
    pauseMenu.propReadonlyExt<bindings::PauseSettingsButton>("settingsButton");
    pauseMenu.propReadonlyExt<bindings::BackButton>("backButton");
    pauseMenu.propReadonlyExt<bindings::ContinueButton>("continueButton");
}
