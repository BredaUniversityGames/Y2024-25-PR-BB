#include "steam_module.hpp"
#include <steam_api.h>

ModuleTickOrder SteamModule::Init(MAYBE_UNUSED Engine& engine)
{
    if(!SteamAPI_Init())
    {
        assert("steam not avaiable!");
    }

    return ModuleTickOrder::ePreTick;
}

void SteamModule::Tick(MAYBE_UNUSED Engine& engine)
{

}

void SteamModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    SteamAPI_Shutdown();
}