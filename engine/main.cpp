#include "main_engine.hpp"
#include "application_module.hpp"
#include "old_engine.hpp"

int main(MAYBE_UNUSED int argc, MAYBE_UNUSED char* argv[])
{
    MainEngine instance;

    instance
        .AddModule<ApplicationModule>()
        .AddModule<OldEngine>();

    return instance.Run();
}
