#include "application_module.hpp"
#include "audio_module.hpp"
#include "main_engine.hpp"
#include "old_engine.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "scripting_module.hpp"
#include "steam_module.hpp"
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
        .AddModule<PhysicsModule>();

    auto& scripting = instance.GetModule<ScriptingModule>();

    // Add modules here to expose them in scripting
    {
        auto& engineAPI = scripting.StartEngineBind();
        scripting.BindModule<TimeModule>(engineAPI, "Time");
        scripting.EndEngineBind(instance);
    }

    scripting.GenerateEngineBindingsFile("game/engine_api.wren");
    instance.GetModule<ScriptingModule>().SetMainScript("game/game.wren");

    return instance.Run();
}
