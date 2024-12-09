#include "audio_module.hpp"

#include
#include

#include <iostream>

#include "fmod_debug.hpp"
#include "fmod_include.hpp"

#include "log.hpp"

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const auto tickOrder = ModuleTickOrder::ePostTick;

    try
    {
        StartFMODDebugLogger();

        FMOD_CHECKRESULT(FMOD_Studio_System_Create(&_studioSystem, FMOD_VERSION));

        FMOD_CHECKRESULT(FMOD_Studio_System_Initialize(_studioSystem, MAX_CHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr));

        FMOD_CHECKRESULT(FMOD_Studio_System_GetCoreSystem(_studioSystem, &_coreSystem));

        FMOD_CHECKRESULT(FMOD_System_GetMasterChannelGroup(_coreSystem, &_masterGroup));
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
        FMOD_CHECKRESULT(FMOD_Studio_System_Release(_studioSystem));
    }

    _coreSystem = nullptr;
    _studioSystem = nullptr;

    bblog::info("FMOD shutdown");
}
void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    FMOD_CHECKRESULT(FMOD_Studio_System_Update(_studioSystem));

    // Clean up events that have stopped playing
    std::vector<uint32_t> eventsToRemove;
    for (auto eventInstance : _events)
    {
        FMOD_STUDIO_PLAYBACK_STATE state;
        FMOD_Studio_EventInstance_GetPlaybackState(eventInstance.second, &state);
        if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
        {
            eventsToRemove.emplace_back(eventInstance.first);
        }
    }

    for (auto id : eventsToRemove)
    {
        _events.erase(id);
    }

    _channelsActive.erase(std::remove_if(_channelsActive.begin(), _channelsActive.end(), [](FMOD_CHANNEL* channel)
                              {   FMOD_BOOL isPlaying = false;
            if (channel)
            {
                FMOD_CHECKRESULT(FMOD_Channel_IsPlaying(channel, &isPlaying));
            }
            return !static_cast<bool>(isPlaying); }),
        _channelsActive.end());
}
void AudioModule::LoadSFX(SoundInfo& soundInfo)
{
    const AudioUID hash = std::hash<std::string_view> {}(soundInfo.path);
    soundInfo.uid = hash;
    if (_sounds.contains(hash))
    {
        return;
    }

    FMOD_MODE mode = soundInfo.isLoop ? FMOD_LOOP_NORMAL : FMOD_DEFAULT;
    mode = soundInfo.is3D ? mode |= FMOD_3D : mode;
    FMOD_SOUND* sound = nullptr;

    FMOD_CHECKRESULT(FMOD_System_CreateSound(_coreSystem, soundInfo.path.data(), mode, nullptr, &sound));

    _sounds[hash] = sound;
}
void AudioModule::PlaySFX(SoundInfo& soundInfo)
{
    if (!_sounds.contains(soundInfo.uid))
    {
        bblog::error("Could not play sound, sound not loaded: {0}", soundInfo.path);
        return;
    }

    FMOD_CHANNEL* channel = nullptr;
    FMOD_CHECKRESULT(FMOD_System_PlaySound(_coreSystem, _sounds[soundInfo.uid], _masterGroup, true, &channel));

    FMOD_CHECKRESULT(FMOD_Channel_SetVolume(channel, soundInfo.volume));

    _channelsActive.emplace(soundInfo.uid, channel);

    FMOD_CHECKRESULT(FMOD_Channel_SetPaused(channel, false));
}
void AudioModule::StopSFX(const SoundInfo& soundInfo)
{
    if (soundInfo.isLoop && _channelsActive.contains(soundInfo.uid))
    {
        FMOD_CHECKRESULT(FMOD_Channel_Stop(_channelsActive[soundInfo.uid]));
        _channelsActive.erase(soundInfo.uid);
    }
}
void AudioModule::LoadBank(BankInfo& bankInfo)
{
    const AudioUID hash = std::hash<std::string_view> {}(bankInfo.path);
    bankInfo.uid = hash;

    if (_banks.contains(hash))
    {
        return;
    }

    FMOD_STUDIO_BANK* bank = nullptr;

    FMOD_CHECKRESULT(FMOD_Studio_System_LoadBankFile(_studioSystem, bankInfo.path.data(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank));

    FMOD_CHECKRESULT(FMOD_Studio_Bank_LoadSampleData(bank));
    FMOD_CHECKRESULT(FMOD_Studio_System_FlushSampleLoading(_studioSystem));

    _banks[hash] = bank;
}
void AudioModule::UnloadBank(const BankInfo& bankInfo)
{
    if (!_banks.contains(bankInfo.uid))
    {
        return;
    }

    FMOD_CHECKRESULT(FMOD_Studio_Bank_Unload(_banks[bankInfo.uid]));
    _banks.erase(bankInfo.uid);
}
AudioUID AudioModule::StartOneShotEvent(std::string_view name)
{
    return StartEvent(name, true);
}
NO_DISCARD AudioUID AudioModule::StartLoopingEvent(std::string_view name)
{
    return StartEvent(name, false);
}
void AudioModule::Update3DSoundPosition(const AudioUID id, const glm::vec3& position)
{
    const auto pos = FMOD_VECTOR(position.x, position.y, position.z);

    FMOD_Channel_Set3DAttributes(_channelsActive[id], pos, nullptr);
}
NO_DISCARD AudioUID AudioModule::StartEvent(std::string_view name, const bool isOneShot)
{
    FMOD_STUDIO_EVENTDESCRIPTION* eve = nullptr;
    FMOD_CHECKRESULT(FMOD_Studio_System_GetEvent(_studioSystem, name.data(), &eve));

    FMOD_STUDIO_EVENTINSTANCE* evi = nullptr;
    FMOD_CHECKRESULT(FMOD_Studio_EventDescription_CreateInstance(eve, &evi));

    const AudioUID eventId = _nextEventId;
    _events[eventId] = evi;
    ++_nextEventId;

    FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Start(evi));

    if (isOneShot)
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Release(evi));
    }

    return eventId;
}
void AudioModule::StopEvent(const AudioUID eventId)
{
    if (_events.contains(eventId))
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Stop(_events[eventId], FMOD_STUDIO_STOP_ALLOWFADEOUT));
    }
}

void AudioModule::SetListener3DAttributes(const glm::vec3& position) const
{
    const auto pos = FMOD_VECTOR(position.x, position.y, position.z);

    FMOD_System_Set3DListenerAttributes(_coreSystem, 0, &pos, nullptr, nullptr, nullptr);
}
