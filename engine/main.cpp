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

int Main()
{
#if defined(DISTRIBUTION)
    bblog::StartWritingToFile();
#endif

    MainEngine instance;
    Stopwatch startupTimer {};

    {
        ZoneScopedN("Engine Module Initialization");

        instance
            .AddModule<ThreadModule>()
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
            .AddModule<GameModule>()
            .AddModule<InspectorModule>();
    }

    {
        ZoneScopedN("Game Script Setup");
        auto& scripting = instance.GetModule<ScriptingModule>();
        scripting.ResetVM();
        scripting.SetMainScript(instance, "game/game.wren");
        instance.GetModule<TimeModule>().ResetTimer();
    }

    bblog::info("{}ms taken for complete startup!", startupTimer.GetElapsed().count());
    return instance.Run();
}

#if defined(_WIN32) && defined(DISTRIBUTION)

int APIENTRY WinMain(MAYBE_UNUSED HINSTANCE hInstance, MAYBE_UNUSED HINSTANCE hPrevInstance, MAYBE_UNUSED LPSTR lpCmdLine, MAYBE_UNUSED int nShowCmd)
{
    return Main();
}

#else

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    return Main();
}

#endif
