#include "analytics_module.hpp"
#include "application_module.hpp"
#include "audio_module.hpp"
#include "ecs_module.hpp"
#include "game_module.hpp"
#include "inspector_module.hpp"
#include "main_engine.hpp"
#include "particle_module.hpp"
#include "pathfinding_module.hpp"
#include "physfs.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "scripting_module.hpp"
#include "steam_module.hpp"
#include "thread_module.hpp"
#include "time_module.hpp"
#include "ui_module.hpp"

int Main()
{
#ifdef DISTRIBUTION
    bblog::StartWritingToFile();
#endif

#ifdef DISTRIBUTION
    fileIO::Init(false); // RAII wrapper for mounting the file system.
#else
    fileIO::Init(true); // RAII wrapper for mounting the file system.
#endif

    int result;
    {
        MainEngine instance;
        Stopwatch startupTimer {};

        {
            ZoneScopedN("Engine Module Initialization");

            instance
                .AddModule<ThreadModule>()
                .AddModule<ECSModule>()
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
                .AddModule<InspectorModule>()
                //.AddModule<AnalyticsModule>()
                .AddModule<ScriptingModule>();
        }

        {
            ZoneScopedN("Game Script Setup");
            auto& scripting = instance.GetModule<ScriptingModule>();

            scripting.ResetVM();
            scripting.GenerateEngineBindingsFile();
            scripting.SetMainScript(instance, "game/main_menu.wren");

            instance.GetModule<TimeModule>().ResetTimer();
        }

        bblog::info("{}ms taken for complete startup!", startupTimer.GetElapsed().count());
        result = instance.Run();
    }

    fileIO::Deinit();
    return result;
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
