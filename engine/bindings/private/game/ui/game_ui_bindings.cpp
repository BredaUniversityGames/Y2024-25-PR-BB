#include "game/ui/game_ui_bindings.hpp"
namespace bindings
{

bool PlayButtonPressedOnce(MainMenu& self)
{
    if (auto locked = self.playButton.lock(); locked != nullptr)
    {
        return locked->IsPressedOnce();
    }
    return false;
}

bool QuitButtonPressedOnce(MainMenu& self)
{
    if (auto locked = self.quitButton.lock(); locked != nullptr)
    {
        return locked->IsPressedOnce();
    }
    return false;
}

}

void BindMainMenu(wren::ForeignModule& module)
{
    auto& mainMenu = module.klass<MainMenu>("MainMenu");
    mainMenu.funcExt<bindings::PlayButtonPressedOnce>("PlayButtonPressedOnce");
    mainMenu.funcExt<bindings::QuitButtonPressedOnce>("QuitButtonPressedOnce");
}
