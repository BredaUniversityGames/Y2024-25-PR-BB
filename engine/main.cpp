#include "application_module.hpp"
#include "audio_module.hpp"
#include "main_engine.hpp"
#include "old_engine.hpp"
#include "particle_module.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "scripting_module.hpp"
#include "steam_module.hpp"
#include "ui_module.hpp"
#include "time_module.hpp"

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    MainEngine instance;

    instance
        .AddModule<ScriptingModule>()
        .AddModule<TimeModule>()
        .AddModule<SteamModule>()
        .AddModule<ApplicationModule>()
        .AddModule<OldEngine>()
        .AddModule<RendererModule>()
        .AddModule<AudioModule>()
        .AddModule<PhysicsModule>()
        .AddModule<UIModule>()
        .AddModule<ParticleModule>();

    auto& scripting = instance.GetModule<ScriptingModule>();

    // Add modules here to expose them in scripting
    {
        auto& engineAPI = scripting.GetEngineClass();
        engineAPI.func<&WrenEngine::GetModule<TimeModule>>("GetTime");
    }

    scripting.GenerateEngineBindingsFile();
    instance.GetModule<ScriptingModule>().SetMainScript(instance, "game/game.wren");

    return instance.Run();
}
