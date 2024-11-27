#include "audio_module.hpp"

#include <iostream>
#include <string>

#include "fmod.h"
#include "fmod_studio.h"

#include "fmod_errors.h"
#include "log.hpp"

#if not defined(NDEBUG)
FMOD_RESULT DebugCallback(FMOD_DEBUG_FLAGS flags, MAYBE_UNUSED const char* file, int line, const char* func, const char* message)
{
    // Get rid of "\n" for better formatting in bblog
    std::string msg(message);
    msg.pop_back();

    if (msg.empty())
    {
        return FMOD_OK;
    }

    // We use std::cout instead of using spdlog because otherwise it crashes 💀 (some threading issue with fmod)
    switch (flags)
    {
    case FMOD_DEBUG_LEVEL_LOG:
        std::cout << "[FMOD INFO] : " << line << " ( " << func << " ) - " << msg << std::endl;
        break;
    case FMOD_DEBUG_LEVEL_WARNING:
        std::cout << "[FMOD WARN] : " << line << " ( " << func << " ) - " << msg << std::endl;
        break;
    case FMOD_DEBUG_LEVEL_ERROR:
        std::cout << "[FMOD ERROR] : " << line << " ( " << func << " ) - " << msg << std::endl;
        break;
    default:
        break;
    }

    return FMOD_OK;
}
#endif

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const ModuleTickOrder tickOrder = ModuleTickOrder::ePostTick;

    FMOD_RESULT result;

#if not defined(NDEBUG)
    // Use FMOD_DEBUG_LEVEL_MEMORY if you want to debug memory issues related to fmod
    result = FMOD_Debug_Initialize(FMOD_DEBUG_LEVEL_LOG, FMOD_DEBUG_MODE_CALLBACK, &DebugCallback, nullptr);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
    }
#endif

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
        FMOD_RESULT result;

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
