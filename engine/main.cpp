#include "application_module.hpp"
#include "audio_module.hpp"
#include "ecs_module.hpp"
#include "game_module.hpp"
#include "inspector_module.hpp"
#include "main_engine.hpp"
#include "old_engine.hpp"
#include "particle_module.hpp"
#include "pathfinding_module.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "scripting_module.hpp"
#include "steam_module.hpp"
#include "thread_module.hpp"
#include "time_module.hpp"
#include "ui_module.hpp"

#include "wren_bindings.hpp"

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    ThreadPool pool { 4 };

    auto wait = []()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    };

    pool.QueueWork(std::packaged_task<void()>(wait));
    pool.Start();
    pool.FinishWork();

    // MainEngine instance;
    //
    // instance
    //     .AddModule<ThreadModule>()
    //     .AddModule<ScriptingModule>()
    //     .AddModule<InspectorModule>()
    //     .AddModule<ECSModule>()
    //     .AddModule<TimeModule>()
    //     .AddModule<SteamModule>()
    //     .AddModule<ApplicationModule>()
    //     .AddModule<PhysicsModule>()
    //     .AddModule<OldEngine>()
    //     .AddModule<RendererModule>()
    //     .AddModule<PathfindingModule>()
    //     .AddModule<AudioModule>()
    //     .AddModule<UIModule>()
    //     .AddModule<ParticleModule>()
    //     .AddModule<GameModule>();
    //
    // auto& scripting = instance.GetModule<ScriptingModule>();
    //
    // BindEngineAPI(scripting.GetForeignAPI());
    // scripting.GenerateEngineBindingsFile();
    // scripting.SetMainScript(instance, "game/game.wren");
    //
    // return instance.Run();
}
