#include "application_module.hpp"
#include "audio_module.hpp"
#include "ecs_module.hpp"
#include "game_module.hpp"
#include "inspector_module.hpp"
#include "main_engine.hpp"
#include "particle_module.hpp"
#include "pathfinding_module.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "scripting_module.hpp"
#include "steam_module.hpp"
#include "thread_module.hpp"
#include "time_module.hpp"
#include "ui_module.hpp"

#include "profile_macros.hpp"
#include "wren_bindings.hpp"

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    MainEngine instance;

    {
        ZoneScopedN("Engine Module Initialization");

        instance
            .AddModule<ThreadModule>()
            .AddModule<InspectorModule>()
            .AddModule<ECSModule>()
            .AddModule<ScriptingModule>()
            .AddModule<TimeModule>()
            .AddModule<SteamModule>()
            .AddModule<ApplicationModule>()
            .AddModule<PhysicsModule>()
            .AddModule<RendererModule>()
            .AddModule<PathfindingModule>()
            .AddModule<AudioModule>()
            .AddModule<UIModule>()
            .AddModule<ParticleModule>()
            .AddModule<GameModule>();
    }

    instance.MainLoopOnce();
    auto& scripting = instance.GetModule<ScriptingModule>();

    BindEngineAPI(scripting.GetForeignAPI());
    scripting.GenerateEngineBindingsFile();

    scripting.SetMainScript(instance, "game/swap_test.wren");

    return instance.Run();
}
