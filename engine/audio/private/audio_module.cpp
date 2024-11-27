#include "audio_module.hpp"

#include <iostream>
#include <string>

#include "fmod.h"
#include "fmod_common.h"
#include "fmod_errors.h"
#include "log.hpp"

void CHECKRESULT_fn(FMOD_RESULT result, MAYBE_UNUSED const char* file, int line)
{
    if (result != FMOD_OK)
        bblog::error("FMOD ERROR: audio_module.cpp [Line {0} ] {1} - {2}", line, static_cast<int>(result), FMOD_ErrorString(result));
}
#define CHECKRESULT(result) CHECKRESULT_fn(result, __FILE__, __LINE__)

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

    CHECKRESULT(FMOD_System_Create(&_fmodSystem, FMOD_VERSION));

#if not defined(NDEBUG)
    // Use FMOD_DEBUG_LEVEL_MEMORY if you want to debug memory issues related to fmod
    CHECKRESULT(FMOD_Debug_Initialize(FMOD_DEBUG_LEVEL_LOG, FMOD_DEBUG_MODE_CALLBACK, &DebugCallback, nullptr));
#endif

    CHECKRESULT(FMOD_System_Init(_fmodSystem, MAX_CHANNELS, FMOD_INIT_NORMAL, nullptr));

    CHECKRESULT(FMOD_System_GetMasterChannelGroup(_fmodSystem, &_masterGroup));

    bblog::info("FMOD initialized successfully");

    return tickOrder;
}
void AudioModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
    if (_fmodSystem)
    {
        CHECKRESULT(FMOD_System_Release(_fmodSystem));
    }

    _fmodSystem = nullptr;

    bblog::info("FMOD shutdown");
}
void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    CHECKRESULT(FMOD_System_Update(_fmodSystem));
}
void AudioModule::LoadSound(SoundInfo& soundInfo)
{
    const uint32_t hash = std::hash<std::string_view> {}(soundInfo.path);
    soundInfo.uid = hash;
    if (_sounds.contains(hash))
    {
        return;
    }

    FMOD_MODE mode = soundInfo.isLoop ? FMOD_LOOP_NORMAL : FMOD_DEFAULT;
    FMOD_SOUND* sound;

    CHECKRESULT(FMOD_System_CreateSound(_fmodSystem, soundInfo.path.data(), mode, nullptr, &sound));

    _sounds[hash] = sound;
    soundInfo.isLoaded = true;
}
void AudioModule::PlaySoundA(SoundInfo& soundInfo)
{
    if (!soundInfo.isLoaded)
    {
        bblog::error("Could not play sound, sound not loaded: {0}", soundInfo.path);
        return;
    }

    FMOD_CHANNEL* channel;
    CHECKRESULT(FMOD_System_PlaySound(_fmodSystem, _sounds[soundInfo.uid], _masterGroup, true, &channel));

    CHECKRESULT(FMOD_Channel_SetVolume(channel, soundInfo.volume));

    if (soundInfo.isLoop)
    {
        _channelsLooping.emplace(soundInfo.uid, channel);
    }

    CHECKRESULT(FMOD_Channel_SetPaused(channel, false));
}
