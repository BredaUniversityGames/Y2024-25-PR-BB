#include "audio_module.hpp"

#include <algorithm>
#include <iostream>

#include "fmod_debug.hpp"
#include "fmod_include.hpp"

#include "audio_system.hpp"
#include "ecs_module.hpp"

#include "log.hpp"

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const auto tickOrder = ModuleTickOrder::ePostTick;

    auto& ecs = engine.GetModule<ECSModule>();
    ecs.AddSystem<AudioSystem>(ecs, *this);

    try
    {
        StartFMODDebugLogger();

        FMOD_CHECKRESULT(FMOD_Studio_System_Create(&_studioSystem, FMOD_VERSION));
        FMOD_CHECKRESULT(FMOD_Studio_System_Initialize(_studioSystem, MAX_CHANNELS, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, nullptr));
        FMOD_CHECKRESULT(FMOD_Studio_System_GetCoreSystem(_studioSystem, &_coreSystem));
        FMOD_CHECKRESULT(FMOD_System_GetMasterChannelGroup(_coreSystem, &_masterGroup));

        // FFT DSP for spectrum debug info
        FMOD_CHECKRESULT(FMOD_System_CreateDSPByType(_coreSystem, FMOD_DSP_TYPE_FFT, &_fftDSP));
        FMOD_CHECKRESULT(FMOD_DSP_SetParameterInt(_fftDSP, FMOD_DSP_FFT_WINDOWSIZE, 512));
        FMOD_CHECKRESULT(FMOD_DSP_SetActive(_fftDSP, true));
        FMOD_CHECKRESULT(FMOD_ChannelGroup_AddDSP(_masterGroup, FMOD_CHANNELCONTROL_DSP_TAIL, _fftDSP));
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

    for (const auto id : eventsToRemove)
    {
        _events.erase(id);
    }

    std::erase_if(_channelsActive, [](const auto& pair)
        {
            FMOD_BOOL isPlaying = false;
            if (pair.second)
            {
                FMOD_Channel_IsPlaying(pair.second, &isPlaying);
            }
            return !static_cast<bool>(isPlaying); });
}
void AudioModule::LoadSFX(SoundInfo& soundInfo)
{
    const SoundID hash = std::hash<std::string_view> {}(soundInfo.path);
    soundInfo.uid = hash;
    if (_sounds.contains(hash) && _soundInfos.contains(hash))
    {
        bblog::error("Could not load sound, sound already loaded: {0}", soundInfo.path);
        return;
    }

    FMOD_MODE mode = soundInfo.isLoop ? FMOD_LOOP_NORMAL : FMOD_DEFAULT;
    mode = soundInfo.is3D ? mode |= FMOD_3D : mode;
    FMOD_SOUND* sound = nullptr;

    FMOD_CHECKRESULT(FMOD_System_CreateSound(_coreSystem, soundInfo.path.data(), mode, nullptr, &sound));

    _sounds[hash] = sound;
    _soundInfos[hash] = &soundInfo;
}
SoundInfo& AudioModule::GetSFX(const std::string_view path)
{
    const SoundInfoID hash = std::hash<std::string_view> {}(path);
    const auto it = std::ranges::find_if(_soundInfos, [&](const std::unordered_map<SoundInfoID, SoundInfo*>::value_type& vt)
        {
        if (vt.second->uid == hash)
        {
            return true;
        }
        return false; });

    return *it->second;
}
SoundInstance AudioModule::PlaySFX(SoundInfo& soundInfo, const float volume, const bool startPaused)
{
    if (!_sounds.contains(soundInfo.uid))
    {
        bblog::error("Could not play sound, sound not loaded: {0}", soundInfo.path);
        return SoundInstance(-1, soundInfo.is3D);
    }

    FMOD_CHANNEL* channel = nullptr;
    FMOD_CHECKRESULT(FMOD_System_PlaySound(_coreSystem, _sounds[soundInfo.uid], _masterGroup, true, &channel));
    FMOD_CHECKRESULT(FMOD_Channel_SetVolume(channel, volume));
    if (!startPaused)
    {
        FMOD_CHECKRESULT(FMOD_Channel_SetPaused(channel, false));
    }
    const ChannelID soundId = _nextSoundId;
    _channelsActive[soundId] = channel;
    ++_nextSoundId;

    return SoundInstance(soundId, soundInfo.is3D);
}
void AudioModule::StopSFX(const SoundInstance instance)
{
    if (_channelsActive.contains(instance.id))
    {
        FMOD_CHECKRESULT(FMOD_Channel_Stop(_channelsActive[instance.id]));
    }
}
void AudioModule::LoadBank(BankInfo& bankInfo)
{
    const BankID hash = std::hash<std::string_view> {}(bankInfo.path);
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
EventInstanceID AudioModule::StartOneShotEvent(std::string_view name)
{
    return StartEvent(name, true);
}
NO_DISCARD EventInstanceID AudioModule::StartLoopingEvent(std::string_view name)
{
    return StartEvent(name, false);
}
void AudioModule::Update3DSoundPosition(const ChannelID id, const glm::vec3& position)
{
    const auto pos = GLMToFMOD(position);
    FMOD_Channel_Set3DAttributes(_channelsActive[id], &pos, nullptr);
}
NO_DISCARD EventInstanceID AudioModule::StartEvent(std::string_view name, const bool isOneShot)
{
    FMOD_STUDIO_EVENTDESCRIPTION* eve = nullptr;
    FMOD_CHECKRESULT(FMOD_Studio_System_GetEvent(_studioSystem, name.data(), &eve));

    FMOD_STUDIO_EVENTINSTANCE* evi = nullptr;
    FMOD_CHECKRESULT(FMOD_Studio_EventDescription_CreateInstance(eve, &evi));

    const EventInstanceID eventId = _nextEventId;
    _events[eventId] = evi;
    ++_nextEventId;

    FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Start(evi));

    if (isOneShot)
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Release(evi));
    }

    return eventId;
}
void AudioModule::StopEvent(const EventInstanceID eventId)
{
    if (_events.contains(eventId))
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Stop(_events[eventId], FMOD_STUDIO_STOP_ALLOWFADEOUT));
    }
}

void AudioModule::SetListener3DAttributes(const glm::vec3& position) const
{
    const auto pos = GLMToFMOD(position);
    FMOD_System_Set3DListenerAttributes(_coreSystem, 0, &pos, nullptr, nullptr, nullptr);
}
