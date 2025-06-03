#pragma once
#include "achievements.hpp"
#include <engine.hpp>
#include <memory>
#include <span>
#include <string>

class SteamModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override;

    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;

    std::string_view GetName() override { return "Steam Module"; }

    bool _steamAvailable = false;
    bool _steamInputAvailable = false;
    // Global access to Achievements object
    CSteamAchievements* m_SteamAchievements = nullptr;

public:
    NON_COPYABLE(SteamModule);
    NON_MOVABLE(SteamModule);

    SteamModule() = default;
    ~SteamModule() override = default;

    void InitSteamAchievements(std::span<Achievement_t> achievements);
    CSteamAchievements& GetSteamAchievements() const { return *m_SteamAchievements; }

    // When the user launched the application through Steam, this will return true.
    // If false, the Steam module cannot be used, as Steam API does not work.
    bool Available() const { return _steamAvailable; }

    // Check first if steam is available before using.
    void OpenSteamBrowser(const std::string& url);

    // Returns whether Steam Input API is available to use.
    bool InputAvailable() const { return _steamInputAvailable; }
};
