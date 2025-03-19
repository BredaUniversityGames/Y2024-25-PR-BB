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
#include <spdlog/sinks/rotating_file_sink.h>
#include <time.h>

int Main()
{
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
    const std::string logFileDir = "logs/";
    const std::string logFileExtension = ".bblog";

    // TODO: Probably good to put the version of the game here as well when we have access to that
    const auto now = std::chrono::system_clock::now();
    const std::string logFileName = std::format("{:%dd-%mm-%Yy_%Hh-%Mm-%OSs}", now);

    const std::string fullName = logFileDir + logFileName + logFileExtension;

    constexpr size_t maxFileSize = 1048576 * 5;
    constexpr size_t maxFiles = 3;

    auto fileLogger = spdlog::rotating_logger_mt("bblog", fullName, maxFileSize, maxFiles);
    bblog::set_default_logger(fileLogger);

    bblog::flush_on(bblog::level::level_enum::trace); // Flush on everything

    return Main();
}

#else

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    return Main();
}

#endif
