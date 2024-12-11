#include "application_module.hpp"
#include "audio_module.hpp"
#include "ecs_module.hpp"
#include "main_engine.hpp"
#include "old_engine.hpp"
#include "particle_module.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "pathfinding_module.h"
#include "scripting_module.hpp"
#include "steam_module.hpp"
#include "time_module.hpp"
#include "inspector_module.hpp"
#include "ui_module.hpp"

#include "wren_bindings.hpp"

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    MainEngine instance;

    instance
        .AddModule<ScriptingModule>()
        .AddModule<InspectorModule>()
        .AddModule<ECSModule>()
        .AddModule<TimeModule>()
        .AddModule<SteamModule>()
        .AddModule<ApplicationModule>()
        .AddModule<PhysicsModule>()
        .AddModule<OldEngine>()
        .AddModule<RendererModule>()
        .AddModule<PathfindingModule>()
        .AddModule<AudioModule>()
        .AddModule<UIModule>()
        .AddModule<ParticleModule>();

    auto& scripting = instance.GetModule<ScriptingModule>();

    BindEngineAPI(scripting.GetForeignAPI());
    scripting.GenerateEngineBindingsFile();
    scripting.SetMainScript(instance, "game/game.wren");

    return instance.Run();
}
