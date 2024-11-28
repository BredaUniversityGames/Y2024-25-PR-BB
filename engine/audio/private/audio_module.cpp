#include "audio_module.hpp"

#include <iostream>
#include <string>

#include "fmod.h"
#include "fmod_studio.h"

#include "fmod_errors.h"
#include "log.hpp"

#include "fmod_debug.hpp"

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const auto tickOrder = ModuleTickOrder::ePostTick;

    try
    {
        StartFMODDebugLogger();

        CHECKRESULT(FMOD_Studio_System_Create(&_studioSystem, FMOD_VERSION));

        CHECKRESULT(FMOD_Studio_System_Initialize(_studioSystem, MAX_CHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr));

        CHECKRESULT(FMOD_Studio_System_GetCoreSystem(_studioSystem, &_coreSystem));

        CHECKRESULT(FMOD_System_GetMasterChannelGroup(_coreSystem, &_masterGroup));
    }
    catch (std::exception& e)
    {
        bblog::error("FMOD did not initialize successfully: {0}", e.what());
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

    const FMOD_MODE mode = soundInfo.isLoop ? FMOD_LOOP_NORMAL : FMOD_DEFAULT;
    FMOD_SOUND* sound;

    CHECKRESULT(FMOD_System_CreateSound(_coreSystem, soundInfo.path.data(), mode, nullptr, &sound));

    _sounds[hash] = sound;
}
void AudioModule::PlaySFX(SoundInfo& soundInfo)
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
void AudioModule::StopSFX(const SoundInfo& soundInfo)
{
    if (soundInfo.isLoop && _channelsLooping.contains(soundInfo.uid))
    {
        CHECKRESULT(FMOD_Channel_Stop(_channelsLooping[soundInfo.uid]));
        _channelsLooping.erase(soundInfo.uid);
    }
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
uint32_t AudioModule::StartOneShotEvent(std::string_view name)
{
    return StartEvent(name, true);
}
uint32_t AudioModule::StartLoopingEvent(std::string_view name)
{
    return StartEvent(name, false);
}
uint32_t AudioModule::StartEvent(const std::string_view name, bool isOneShot)
{
    FMOD_STUDIO_EVENTDESCRIPTION* eve;
    CHECKRESULT(FMOD_Studio_System_GetEvent(_studioSystem, name.data(), &eve));

    FMOD_STUDIO_EVENTINSTANCE* evi;
    CHECKRESULT(FMOD_Studio_EventDescription_CreateInstance(eve, &evi));

    const uint32_t eventId = _nextEventId;
    _events[eventId] = evi;
    ++_nextEventId;

    CHECKRESULT(FMOD_Studio_EventInstance_Start(evi));

    if (isOneShot)
    {
        CHECKRESULT(FMOD_Studio_EventInstance_Release(evi));
    }

    return eventId;
}
void AudioModule::StopEvent(const uint32_t eventId)
{
    if (_events.contains(eventId))
    {
        CHECKRESULT(FMOD_Studio_EventInstance_Stop(_events[eventId], FMOD_STUDIO_STOP_ALLOWFADEOUT));
    }
}
