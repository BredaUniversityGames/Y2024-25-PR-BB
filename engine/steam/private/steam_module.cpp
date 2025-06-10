#include "steam_module.hpp"
#include "log.hpp"
#include "steam_include.hpp"

void DebugCallback(int severity, const char* message)
{
    // If you're running in the debugger, only warnings (severity >= 1) will be sent
    // If you add -debug_steamapi to the command-line, a lot of extra informational messages will also be sent

    switch (severity)
    {
    case 0:
        bblog::info("[Steamworks] ", message);
        break;
    case 1:
        bblog::warn("[Steamworks] ", message);
        break;
    default:
        bblog::error("[Steamworks] ", message);
    }
}

ModuleTickOrder SteamModule::Init(MAYBE_UNUSED Engine& engine)
{
    SteamErrMsg errorMessage = { 0 };
    if (SteamAPI_InitEx(&errorMessage) != k_ESteamAPIInitResult_OK)
    {
        bblog::error("[Steamworks] ", errorMessage);
        bblog::warn("[Steamworks] Steam is probably not running, Steamworks API functionality will be unavailable");

        return ModuleTickOrder::ePreTick;
    }

    SteamClient()->SetWarningMessageHook(&DebugCallback);

    if (!SteamUser()->BLoggedOn())
    {
        bblog::warn("[Steamworks] Steam user is not logged in, Steamworks API functionality will be unavailable");

        return ModuleTickOrder::ePreTick;
    }

    _steamAvailable = true;

    if (!SteamInput()->Init(false))
    {
        bblog::error("[Steamworks] Failed to initialize Steam Input");

        return ModuleTickOrder::ePreTick;
    }

    _steamInputAvailable = true;
    SteamAPI_RunCallbacks();

    return ModuleTickOrder::ePreTick;
}

void SteamModule::Tick(MAYBE_UNUSED Engine& engine)
{
    if (!_steamAvailable)
    {
        return;
    }

    SteamAPI_RunCallbacks();
}

void SteamModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    SteamAPI_Shutdown();
}
void SteamModule::OpenSteamBrowser(const std::string& url)
{
    if (_steamAvailable == false)
    {
        bblog::error("Steam is not available, cannot open Steam browser.");
    }
    else
    {
        SteamFriends()->ActivateGameOverlayToWebPage(url.c_str());
    }
}