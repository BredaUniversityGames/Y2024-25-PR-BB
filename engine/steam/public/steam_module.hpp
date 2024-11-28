#pragma once
#include <engine.hpp>
#include <memory>

class SteamModule : public ModuleInterface
{
	ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override;

    void Tick(MAYBE_UNUSED Engine& engine) override;
    void Shutdown(MAYBE_UNUSED Engine& engine) override;

    bool _steamAvailable = false;

public:
    NON_COPYABLE(SteamModule);
    NON_MOVABLE(SteamModule);

    SteamModule() = default;
    ~SteamModule() override = default;

    // When the user launched the application through Steam, this will return true.
    // If false, the steam module cannot be used, as Steam API does not work.
    bool Available() const { return _steamAvailable; }
};
