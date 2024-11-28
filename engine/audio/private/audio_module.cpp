#include "audio_module.hpp"

#include <iostream>
#include <string>

#include "fmod.h"
#include "fmod_studio.h"

#include "fmod_errors.h"
#include "log.hpp"

#include "fmod_debug.hpp"
void CHECKRESULT_fn(FMOD_RESULT result, MAYBE_UNUSED const char* file, int line)
{
    if (result != FMOD_OK)
        bblog::error("FMOD ERROR: audio_module.cpp [Line {0} ] {1} - {2}", line, static_cast<int>(result), FMOD_ErrorString(result));
}
#define CHECKRESULT(result) CHECKRESULT_fn(result, __FILE__, __LINE__)


ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const ModuleTickOrder tickOrder = ModuleTickOrder::ePostTick;


    StartFMODDebugLogger();
    FMOD_RESULT result;

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
    if (_studioSystem)
    {
        CHECKRESULT(FMOD_Studio_System_Release(_studioSystem));
    }

    _coreSystem = nullptr;
    _studioSystem = nullptr;

    bblog::info("FMOD shutdown");
}
void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    CHECKRESULT(FMOD_Studio_System_Update(_studioSystem));

    // Clean up events that have stopped playing
    std::vector<uint32_t> eventsToRemove;
    for (auto eventInstance : _events)
    {
        FMOD_STUDIO_PLAYBACK_STATE state;
        CHECKRESULT(FMOD_Studio_EventInstance_GetPlaybackState(eventInstance.second, &state));
        if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
        {
            eventsToRemove.emplace_back(eventInstance.first);
        }
    }

    for (auto id : eventsToRemove)
    {
        _events.erase(id);
    }
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

    CHECKRESULT(FMOD_System_CreateSound(_coreSystem, soundInfo.path.data(), mode, nullptr, &sound));

    _sounds[hash] = sound;
}
void AudioModule::PlaySoundA(SoundInfo& soundInfo)
{
    if (!_sounds.contains(soundInfo.uid))
    {
        bblog::error("Could not play sound, sound not loaded: {0}", soundInfo.path);
        return;
    }

    FMOD_CHANNEL* channel;
    CHECKRESULT(FMOD_System_PlaySound(_coreSystem, _sounds[soundInfo.uid], _masterGroup, true, &channel));

    CHECKRESULT(FMOD_Channel_SetVolume(channel, soundInfo.volume));

    if (soundInfo.isLoop)
    {
        _channelsLooping.emplace(soundInfo.uid, channel);
    }

    CHECKRESULT(FMOD_Channel_SetPaused(channel, false));
}
void AudioModule::LoadBank(BankInfo& bankInfo)
{
    const uint32_t hash = std::hash<std::string_view> {}(bankInfo.path);
    bankInfo.uid = hash;

    if (_banks.contains(hash))
    {
        return;
    }

    FMOD_STUDIO_BANK* bank;

    CHECKRESULT(FMOD_Studio_System_LoadBankFile(_studioSystem, bankInfo.path.data(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank));

    CHECKRESULT(FMOD_Studio_Bank_LoadSampleData(bank));
    CHECKRESULT(FMOD_Studio_System_FlushSampleLoading(_studioSystem));

    _banks[hash] = bank;
}
void AudioModule::UnloadBank(const BankInfo& bankInfo)
{
    if (!_banks.contains(bankInfo.uid))
    {
        return;
    }

    CHECKRESULT(FMOD_Studio_Bank_Unload(_banks[bankInfo.uid]));
    _banks.erase(bankInfo.uid);
}
uint32_t AudioModule::StartEvent(std::string_view name)
{
    FMOD_STUDIO_EVENTDESCRIPTION* eve;
    CHECKRESULT(FMOD_Studio_System_GetEvent(_studioSystem, name.data(), &eve));

    FMOD_STUDIO_EVENTINSTANCE* evi;
    CHECKRESULT(FMOD_Studio_EventDescription_CreateInstance(eve, &evi));

    uint32_t eventId = _nextEventId;
    _events[eventId] = evi;
    ++_nextEventId;

    CHECKRESULT(FMOD_Studio_EventInstance_Start(evi));

    // CHECKRESULT(FMOD_Studio_EventInstance_Release(evi));

    return eventId;
}
