#include "audio_module.hpp"

#include <iostream>
#include <string>

#include "fmod.h"
#include "fmod_common.h"
#include "fmod_errors.h"
#include "log.hpp"

#if not defined(NDEBUG)
FMOD_RESULT DebugCallback(FMOD_DEBUG_FLAGS flags, MAYBE_UNUSED const char* file, int line, const char* func, const char* message)
{
    // Get rid of "\n" for better formatting in bblog
    std::string msg(message);
    msg.pop_back();

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

    result = FMOD_System_Create(&_fmodSystem, FMOD_VERSION);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
        return tickOrder;
    }

#if not defined(NDEBUG)
    // Use FMOD_DEBUG_LEVEL_MEMORY if you want to debug memory issues related to fmod
    result = FMOD_Debug_Initialize(FMOD_DEBUG_LEVEL_LOG, FMOD_DEBUG_MODE_CALLBACK, &DebugCallback, nullptr);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
    }
#endif

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
