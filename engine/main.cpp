#include "application_module.hpp"
#include "audio_module.hpp"
#include "main_engine.hpp"
#include "old_engine.hpp"
#include "physics_module.hpp"
#include "renderer_module.hpp"
#include "steam_module.hpp"
#include "particle_module.hpp"

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    MainEngine instance;

    instance
        .AddModule<SteamModule>()
        .AddModule<ApplicationModule>()
        .AddModule<OldEngine>()
        .AddModule<RendererModule>()
        .AddModule<AudioModule>()
        .AddModule<PhysicsModule>()
        .AddModule<ParticleModule>();

    return instance.Run();
}
