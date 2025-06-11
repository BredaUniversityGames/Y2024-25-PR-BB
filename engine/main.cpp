#include "analytics_module.hpp"
#include "application_module.hpp"
#include "audio_module.hpp"
#include "ecs_module.hpp"
#include "game_module.hpp"
#include "inspector_module.hpp"
#include "main_engine.hpp"
#include "particle_module.hpp"
#include "pathfinding_module.hpp"
#include "physics_module.hpp"
#include "profile_macros.hpp"
#include "renderer_module.hpp"
#include "scripting_module.hpp"
#include "steam_module.hpp"
#include "thread_module.hpp"
#include "time_module.hpp"
#include "ui_module.hpp"

#include "physfs.hpp"

int Main()
{
#ifdef DISTRIBUTION
    bblog::StartWritingToFile();
#endif

    if (!PhysFS::init(""))
    {
        bblog::error("Failed initializing PhysFS!\n{}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return 1;
    }

    if (!PhysFS::mount("./", "/", true))
    {
        bblog::error("Failed mounting PhysFS!\n{}", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
        return 1;
    }

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
