#include "old_engine.hpp"
#include "sdl_app.hpp"
#include <filesystem>
#include <spdlog/spdlog.h>

#if defined(TESTS_ENABLED)
#undef None
#undef Bool
#include <gtest/gtest.h>
#endif

std::shared_ptr<Application> g_app;
std::unique_ptr<OldEngine> g_engine;

int main(int argc, char* argv[])
{
#if defined(TESTS_ENABLED)
    if (argc > 1 && std::strcmp(argv[1], "-T") == 0)
    {
        testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
#endif

    Application::CreateParameters parameters { "Vulkan", true };

    g_app = std::make_shared<SDLApp>(parameters);

    g_engine = std::make_unique<OldEngine>(g_app->GetInitInfo(), g_app);

    try
    {
        g_engine->Run();
    }
    catch (const std::exception& e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    g_engine.reset();

    return EXIT_SUCCESS;
}
