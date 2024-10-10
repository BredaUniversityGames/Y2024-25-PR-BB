// #include <pch.hpp>
// #include "old_engine.hpp"
// #include "sdl_app.hpp"
// #include <filesystem>
// #include <spdlog/spdlog.h>

//#include "pch.hpp"
#include "main_engine.hpp"
#include "application_module.hpp"
#include "vulkan/vulkan.hpp"
//#include "old_engine.hpp"

// int main(int argc, char* argv[])
// {
//     std::shared_ptr<Application> g_app;
//     std::unique_ptr<OldEngine> g_engine;
//
//     Application::CreateParameters parameters { "Vulkan", true };
//
//     g_app = std::make_shared<SDLApp>(parameters);
//
//     g_engine = std::make_unique<OldEngine>(g_app->GetInitInfo(), g_app);
//
//     try
//     {
//         g_engine->Run();
//     }
//     catch (const std::exception& e)
//     {
//         spdlog::error(e.what());
//         return EXIT_FAILURE;
//     }
//
//     g_engine.reset();
//
//     return EXIT_SUCCESS;
// }

int main(int argc, char* argv[])
{
    MainEngine instance;

    instance
        .AddModule<ApplicationModule>()
    ;

    return instance.Run();
}
