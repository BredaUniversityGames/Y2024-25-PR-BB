#include "engine.hpp"

#include "sdl_app.hpp"

#include <memory>

std::shared_ptr<Application> g_app;
std::unique_ptr<Engine> g_engine;

int main(int argc, const char** argv)
{
    Application::CreateParameters parameters{ "Vulkan", true };

    g_app = std::make_shared<SDLApp>(parameters);

    g_engine = std::make_unique<Engine>(g_app->GetInitInfo(), g_app);

    try
    {
        g_app->Run([]() { g_engine->Run(); return g_engine->ShouldQuit(); });
    }
    catch (const std::exception& e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    g_engine.reset();

    return EXIT_SUCCESS;
}

