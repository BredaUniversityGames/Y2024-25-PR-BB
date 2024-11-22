#include "audio_module.hpp"

#include "fmod.h"
#include "fmod_errors.h"
#include "log.hpp"
#include <memory>

AudioModule::AudioModule()
{
}

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const ModuleTickOrder tickOrder = ModuleTickOrder::ePostTick;

    FMOD_SYSTEM* rawSystem = nullptr;

    FMOD_RESULT result = FMOD_System_Create(&rawSystem, 0x00020223);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        return tickOrder;
    }

    system = std::unique_ptr<FMOD_SYSTEM>(rawSystem);

    result = FMOD_System_Init(system.get(), 512, FMOD_INIT_NORMAL, nullptr);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        FMOD_System_Release(system.get());
        return tickOrder;
    }

    return tickOrder;
}
void AudioModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    if (system)
        FMOD_System_Release(system.get());
}
void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    FMOD_RESULT result = FMOD_System_Update(system.get());

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
    }
}
