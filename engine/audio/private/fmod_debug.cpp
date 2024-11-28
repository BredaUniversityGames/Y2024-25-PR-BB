#include "fmod_debug.hpp"

#include "fmod.h"
#include "fmod_errors.h"
#include "log.hpp"

#include <common.hpp>
#include <iostream>
#include <string>

#if not defined(NDEBUG)
FMOD_RESULT DebugCallback(FMOD_DEBUG_FLAGS flags, MAYBE_UNUSED const char* file, int line, const char* func, const char* message)
{
    // We use std::cout instead of using spdlog because otherwise it crashes 💀 (some threading issue with fmod)
    switch (flags)
    {
    case FMOD_DEBUG_LEVEL_LOG:
        std::cout << "[FMOD INFO] : " << line << " ( " << func << " ) - " << message << std::flush;
        break;
    case FMOD_DEBUG_LEVEL_WARNING:
        std::cout << "[FMOD WARN] : " << line << " ( " << func << " ) - " << message << std::flush;
        break;
    case FMOD_DEBUG_LEVEL_ERROR:
        std::cout << "[FMOD ERROR] : " << line << " ( " << func << " ) - " << message << std::flush;
        break;
    default:
        break;
    }

    return FMOD_OK;
}
#endif

void StartFMODDebugLogger()
{
#if not defined(NDEBUG)
    // Use FMOD_DEBUG_LEVEL_MEMORY if you want to debug memory issues related to fmod
    FMOD_RESULT result = FMOD_Debug_Initialize(FMOD_DEBUG_LEVEL_LOG, FMOD_DEBUG_MODE_CALLBACK, &DebugCallback, nullptr);

    if (result != FMOD_OK)
    {
        bblog::error("FMOD Error: {0}", FMOD_ErrorString(result));
    }
#endif
}