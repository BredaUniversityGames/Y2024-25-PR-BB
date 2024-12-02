#include "audio_module.hpp"

#include "fmod_debug.hpp"
#include "log.hpp"

#include <fmod_include.hpp>

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const ModuleTickOrder tickOrder = ModuleTickOrder::ePostTick;

    FMOD_RESULT result {};

    StartFMODDebugLogger();

    result = FMOD_Studio_System_Create(&_studioSystem, FMOD_VERSION);
    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        return tickOrder;
    }

    result = FMOD_Studio_System_Initialize(_studioSystem, 512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        return tickOrder;
    }

    result = FMOD_Studio_System_GetCoreSystem(_studioSystem, &_coreSystem);
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
    if (_coreSystem)
    {
        FMOD_RESULT result {};

        result = FMOD_Studio_System_Release(_studioSystem);
        if (result != FMOD_OK)
        {
            bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        }
    }

    _coreSystem = nullptr;

    bblog::info("FMOD shutdown");
}
void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    FMOD_RESULT result = FMOD_System_Update(_coreSystem);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
    }
}
