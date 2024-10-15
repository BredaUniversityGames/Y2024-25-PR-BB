#include "main_engine.hpp"
#include "application_module.hpp"

// TODO: Remove the need to include the pch here
#include "pch.hpp"
#include "old_engine.hpp"

int main(int argc, char* argv[])
{
    std::this_thread::sleep_for(std::chrono::seconds(10));
    MainEngine instance;

    instance
        .AddModule<ApplicationModule>()
        .AddModule<OldEngine>();

    return instance.Run();
}
