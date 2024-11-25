#include "application_module.hpp"
#include "main_engine.hpp"
#include "old_engine.hpp"
#include "audio_module.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    MainEngine instance;

    instance
        .AddModule<ApplicationModule>()
        .AddModule<OldEngine>()
        .AddModule<RendererModule>()
        .AddModule<AudioModule>()
        .AddModule<RendererModule>()
        .AddModule<PhysicsModule>();

    return instance.Run();
}
