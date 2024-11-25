#include "audio_module.hpp"

#include "fmod.h"
#include "fmod_common.h"
#include "fmod_errors.h"
#include "log.hpp"

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const ModuleTickOrder tickOrder = ModuleTickOrder::ePostTick;

    FMOD_RESULT result = FMOD_System_Create(&_fmodSystem, FMOD_VERSION);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        return tickOrder;
    }

    result = FMOD_System_Init(_fmodSystem, 512, FMOD_INIT_NORMAL, nullptr);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        return tickOrder;
    }

    bblog::info("FMOD initialized successfully");

    return tickOrder;
}
void AudioModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    if (_fmodSystem)
    {
        FMOD_RESULT result;

        result = FMOD_System_Close(_fmodSystem);
        if (result != FMOD_OK)
        {
            bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        }

        result = FMOD_System_Release(_fmodSystem);
        if (result != FMOD_OK)
        {
            bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        }
    }

    _fmodSystem = nullptr;

    bblog::info("FMOD shutdown");
}
void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    FMOD_RESULT result = FMOD_System_Update(_fmodSystem);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
    }
}
