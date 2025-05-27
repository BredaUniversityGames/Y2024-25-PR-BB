#include "audio_module.hpp"

#include <algorithm>
#include <iostream>

#include "fmod_debug.hpp"
#include "fmod_include.hpp"

#include "audio_system.hpp"
#include "ecs_module.hpp"
#include "physics_module.hpp"

#include "log.hpp"

inline FMOD_VECTOR GLMToFMOD(const glm::vec3& v)
{
    FMOD_VECTOR vec;
    vec.x = v.x;
    vec.y = v.y;
    vec.z = v.z;

    return vec;
}

ModuleTickOrder AudioModule::Init(MAYBE_UNUSED Engine& engine)
{
    const auto tickOrder = ModuleTickOrder::ePostTick;

    auto& ecs = engine.GetModule<ECSModule>();
    ecs.AddSystem<AudioSystem>(ecs, *this);

    _physics = &engine.GetModule<PhysicsModule>();

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

void AudioModule::Reset()
{
    for (auto& [_, event] : _events)
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Stop(event, FMOD_STUDIO_STOP_IMMEDIATE));
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Release(event));
    }

    for (auto& [_, channel] : _channelsActive)
    {
        FMOD_CHECKRESULT(FMOD_Channel_Stop(channel));
    }

    soundsToPlay = {};
    _channelsActive.clear();
    _events.clear();
}

void AudioModule::Tick(MAYBE_UNUSED Engine& engine)
{
    FMOD_CHECKRESULT(FMOD_Studio_System_Update(_studioSystem));

    std::erase_if(_events, [](const auto& pair)
        {
        FMOD_STUDIO_PLAYBACK_STATE state {};
        FMOD_Studio_EventInstance_GetPlaybackState(pair.second, &state);
        if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
        {
            return true;
        }
        return false; });

    std::erase_if(_channelsActive, [](const auto& pair)
        {
            FMOD_BOOL isPlaying = false;
            if (pair.second)
            {
                FMOD_Channel_IsPlaying(pair.second, &isPlaying);
            }
            return !static_cast<bool>(isPlaying); });

    while (soundsToPlay.size())
    {
        auto sound = soundsToPlay.front();
        soundsToPlay.pop();

        FMOD_CHECKRESULT(FMOD_Channel_SetPaused(_channelsActive[sound], false));
    }
}
SoundID AudioModule::LoadSFX(SoundInfo& soundInfo)
{
    const SoundID hash = std::hash<std::string_view> {}(soundInfo.path);
    soundInfo.uid = hash;
    if (_sounds.contains(hash) && _soundInfos.contains(soundInfo.path.data()))
    {
        bblog::warn("Sound at path already loaded: {0}", soundInfo.path);
        return hash;
    }

    FMOD_MODE mode = soundInfo.isLoop ? FMOD_LOOP_NORMAL : FMOD_DEFAULT;
    mode = soundInfo.is3D ? mode |= FMOD_3D : mode;
    FMOD_SOUND* sound = nullptr;
    FMOD_CHECKRESULT(FMOD_System_CreateSound(_coreSystem, soundInfo.path.data(), mode, nullptr, &sound));

    _sounds[hash] = sound;
    _soundInfos[soundInfo.path.data()] = soundInfo;

    return hash;
}
SoundID AudioModule::GetSFX(const std::string_view path)
{
    if (const auto it = _soundInfos.find(path.data()); it != _soundInfos.end())
    {
        return it->second.uid;
    }
    return -1;
}
SoundInstance AudioModule::PlaySFX(SoundID id, const float volume, const bool startPaused)
{
    if (!_sounds.contains(id))
    {
        bblog::error("Could not play sound, sound not loaded: {0}", id);
        return SoundInstance(-1, false);
    }

    FMOD_CHANNEL* channel = nullptr;
    FMOD_MODE mode {};
    FMOD_CHECKRESULT(FMOD_System_PlaySound(_coreSystem, _sounds[id], _masterGroup, true, &channel));
    FMOD_CHECKRESULT(FMOD_Sound_GetMode(_sounds[id], &mode));
    FMOD_CHECKRESULT(FMOD_Channel_SetVolume(channel, volume));

    const ChannelID channelID = _nextSoundId;
    _channelsActive[channelID] = channel;
    ++_nextSoundId;

    SetPaused(channelID, startPaused);

    return SoundInstance(channelID, mode | FMOD_3D);
}
void AudioModule::SetPaused(ChannelID channelId, const bool paused)
{
    if (_channelsActive.contains(channelId))
    {
        if (paused)
        {
            FMOD_CHECKRESULT(FMOD_Channel_SetPaused(_channelsActive[channelId], true));
        }
        else
        {
            soundsToPlay.emplace(channelId);
        }
    }
}
void AudioModule::StopSFX(const SoundInstance instance)
{
    if (_channelsActive.contains(instance.id))
    {
        FMOD_CHECKRESULT(FMOD_Channel_Stop(_channelsActive[instance.id]));
    }
}
bool AudioModule::IsSFXPlaying(const SoundInstance instance)
{
    FMOD_BOOL isPlaying = false;
    if (_channelsActive.contains(instance.id))
    {
        FMOD_Channel_IsPlaying(_channelsActive[instance.id], &isPlaying);

        return isPlaying;
    }
    return isPlaying;
}
bool AudioModule::isSFXLoaded(const std::string_view path) const
{
    return _soundInfos.contains(path.data());
}
void AudioModule::LoadBank(std::string_view path)
{
    const BankID hash = std::hash<std::string_view> {}(path);

    if (_banks.contains(hash))
    {
        return;
    }

    FMOD_STUDIO_BANK* bank = nullptr;
    FMOD_CHECKRESULT(FMOD_Studio_System_LoadBankFile(_studioSystem, path.data(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank));

    FMOD_CHECKRESULT(FMOD_Studio_Bank_LoadSampleData(bank));
    FMOD_CHECKRESULT(FMOD_Studio_System_FlushSampleLoading(_studioSystem));

    _banks[hash] = bank;
}

void AudioModule::RegisterChannelBus(const std::string& busName)
{
    FMOD_CHECKRESULT(FMOD_Studio_System_GetBus(_studioSystem, busName.c_str(), &_eventBusMap[busName]));
}

void AudioModule::SetBusChannelVolume(const std::string& name, float value)
{
    if (auto it = _eventBusMap.find(name); it != _eventBusMap.end())
    {
        FMOD_CHECKRESULT(FMOD_Studio_Bus_SetVolume(it->second, std::clamp(value, 0.0f, 64.0f)));
    }
    else
    {
        bblog::warn("No event bus with the name {}.", name);
    }
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
EventInstance AudioModule::StartOneShotEvent(const std::string_view name)
{
    return StartEvent(name, true);
}
NO_DISCARD EventInstance AudioModule::StartLoopingEvent(const std::string_view name)
{
    return StartEvent(name, false);
}
void AudioModule::UpdateSound3DAttributes(const ChannelID id, const glm::vec3& position, const glm::vec3& velocity)
{
    if (!_channelsActive.contains(id))
    {
        bblog::warn("Tried to update 3d attributes of sound that isn't playing");
        return;
    }

    const auto pos = GLMToFMOD(position);
    const auto vel = GLMToFMOD(velocity);
    FMOD_Channel_Set3DAttributes(_channelsActive[id], &pos, &vel);
}

bool HasDistanceParameter(FMOD_STUDIO_EVENTDESCRIPTION* eventDescription)
{
    int paramCount = 0;
    FMOD_Studio_EventDescription_GetParameterDescriptionCount(eventDescription, &paramCount);

    for (int i = 0; i < paramCount; ++i)
    {
        FMOD_STUDIO_PARAMETER_DESCRIPTION paramDesc;
        FMOD_CHECKRESULT(FMOD_Studio_EventDescription_GetParameterDescriptionByIndex(eventDescription, i, &paramDesc));
        if (std::string(paramDesc.name) == "Distance")
        {
            return true;
        }
    }
    return false;
}

NO_DISCARD EventInstance AudioModule::StartEvent(const std::string_view name, const bool isOneShot)
{
    FMOD_STUDIO_EVENTDESCRIPTION* eve = nullptr;
    FMOD_CHECKRESULT(FMOD_Studio_System_GetEvent(_studioSystem, name.data(), &eve));
    FMOD_STUDIO_EVENTINSTANCE* evi = nullptr;
    FMOD_CHECKRESULT(FMOD_Studio_EventDescription_CreateInstance(eve, &evi));

    const EventInstanceID eventId = _nextEventId;
    _events[eventId] = evi;
    ++_nextEventId;

    const bool startLater = HasDistanceParameter(eve);

    if (!startLater)
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Start(evi));
        if (isOneShot)
        {
            FMOD_BOOL fmodOneShot = false;
            FMOD_CHECKRESULT(FMOD_Studio_EventDescription_IsOneshot(eve, &fmodOneShot));
            if (!fmodOneShot)
            {
                FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Stop(evi, FMOD_STUDIO_STOP_ALLOWFADEOUT));
            }
            FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Release(evi));
        }
    }

    return EventInstance(eventId, startLater, isOneShot);
}
void AudioModule::StopEvent(const EventInstance instance)
{
    if (_events.contains(instance.id))
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Stop(_events[instance.id], FMOD_STUDIO_STOP_ALLOWFADEOUT));
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Release(_events[instance.id]));
    }
}
bool AudioModule::IsEventPlaying(EventInstance instance)
{
    if (_events.contains(instance.id))
    {
        if (instance.startDuringSystemUpdate)
        {
            return true;
        }
        FMOD_STUDIO_PLAYBACK_STATE state {};
        FMOD_Studio_EventInstance_GetPlaybackState(_events[instance.id], &state);

        return state != FMOD_STUDIO_PLAYBACK_STOPPED;
    }
    return false;
}

void AudioModule::SetEventVolume(EventInstance ev, float volume)
{
    if (const auto it = _events.find(ev.id); it != _events.end())
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_SetVolume(it->second, volume));
    }
}

void AudioModule::SetEventFloatAttribute(EventInstance ev, const std::string& name, float val)
{
    if (const auto it = _events.find(ev.id); it != _events.end())
    {
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_SetParameterByName(it->second, name.c_str(), val, false));
    }
}

void AudioModule::SetListener3DAttributes(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up) const
{
    FMOD_3D_ATTRIBUTES attribs {};
    attribs.position = GLMToFMOD(position);
    attribs.velocity = GLMToFMOD(velocity);
    attribs.forward = GLMToFMOD(glm::normalize(forward));
    attribs.up = GLMToFMOD(glm::normalize(up));

    FMOD_Studio_System_SetListenerAttributes(_studioSystem, 0, &attribs, nullptr);
}

void AudioModule::SetEvent3DAttributes(EventInstance instance, const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up)
{
    if (!_events.contains(instance.id))
    {
        bblog::warn("Tried to update event 3d attributes, of event that isn't playing");
        return;
    }

    FMOD_3D_ATTRIBUTES attribs;
    attribs.position = GLMToFMOD(position);
    attribs.velocity = GLMToFMOD(velocity);
    attribs.forward = GLMToFMOD(forward);
    attribs.up = GLMToFMOD(up);

    FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Set3DAttributes(_events[instance.id], &attribs));
}
void AudioModule::StartEventInstance(const EventInstance instance)
{
    FMOD_STUDIO_EVENTDESCRIPTION* eve;
    FMOD_CHECKRESULT(FMOD_Studio_EventInstance_GetDescription(_events[instance.id], &eve));
    FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Start(_events[instance.id]));

    if (instance.isOneShot)
    {
        FMOD_BOOL fmodOneShot = false;
        FMOD_CHECKRESULT(FMOD_Studio_EventDescription_IsOneshot(eve, &fmodOneShot));
        if (!fmodOneShot)
        {
            FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Stop(_events[instance.id], FMOD_STUDIO_STOP_ALLOWFADEOUT));
        }
        FMOD_CHECKRESULT(FMOD_Studio_EventInstance_Release(_events[instance.id]));
    }
}