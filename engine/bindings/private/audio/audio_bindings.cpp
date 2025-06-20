#include "audio_bindings.hpp"

#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "log.hpp"
#include "wren_entity.hpp"

namespace bindings
{
void LoadBank(AudioModule& self, const std::string& path)
{
    self.LoadBank(path);
}

void LoadSFX(AudioModule& self, const std::string& path, const bool is3D, const bool isLoop)
{
    SoundInfo si {};

    si.path = path;
    si.is3D = is3D;
    si.isLoop = isLoop;

    self.LoadSFX(si);
}

std::optional<SoundInstance> PlaySFX(AudioModule& self, const std::string& path, const float volume)
{
    if (!self.isSFXLoaded(path))
    {
        bblog::error("Tried to play a sound that was not loaded: {0}", path);
        return std::nullopt;
    }

    return self.PlaySFX(self.GetSFX(path), volume, false);
}

void StopSFX(AudioModule& self, const SoundInstance& sound_instance)
{
    self.StopSFX(sound_instance);
}

bool IsSFXPlaying(AudioModule& self, const SoundInstance instance)
{
    return self.IsSFXPlaying(instance);
}

bool IsEventPlaying(AudioModule& self, const EventInstance instance)
{
    return self.IsEventPlaying(instance);
}

EventInstance PlayEventOnce(AudioModule& self, const std::string& path)
{
    return self.StartOneShotEvent(path);
}

EventInstance PlayEventLoop(AudioModule& self, const std::string& path)
{
    return self.StartLoopingEvent(path);
}

void StopEvent(AudioModule& self, EventInstance instance)
{
    self.StopEvent(instance);
}

void SetEventVolume(AudioModule& self, EventInstance instance, float val)
{
    self.SetEventVolume(instance, val);
}

void AddSFX(WrenComponent<AudioEmitterComponent>& self, SoundInstance& instance)
{
    self.component->_soundIds.emplace_back(instance);
}

void AddEvent(WrenComponent<AudioEmitterComponent>& self, EventInstance& instance)
{
    self.component->_eventIds.emplace_back(instance);
}

void SetEventFloatAttribute(AudioModule& self, EventInstance audio, const std::string& name, float val)
{
    self.SetEventFloatAttribute(audio, name, val);
}

void EnableLowPass(AudioModule& self)
{
    self.SetLowpassBypass(false);
}

void DisableLowPass(AudioModule& self)
{
    self.SetLowpassBypass(true);
}

void SetMasterChannelGroupPitch(AudioModule& self, float pitch)
{
    self.SetMasterChannelGroupPitch(pitch);
}

}

void BindAudioAPI(wren::ForeignModule& module)
{
    auto& wren_class = module.klass<AudioModule>("Audio", "Manages Audio 2D and 3D audio output and loading");
    wren_class.funcExt<bindings::LoadBank>("LoadBank", "Loads a FMOD audio bank internally with path provided");
    wren_class.funcExt<bindings::LoadSFX>("LoadSFX", "Loads an audio file internally with path provided, bool is3D and bool isLooping");
    wren_class.funcExt<bindings::PlaySFX>("PlaySFX", "Plays a previously loaded sound with provided path and volume, returns a sound instance");
    wren_class.funcExt<bindings::IsSFXPlaying>("IsSFXPlaying", "Checks if a specific sound instance is still playing");
    wren_class.funcExt<bindings::IsEventPlaying>("IsEventPlaying", "Checks if a specific event instance is still playing");
    wren_class.funcExt<bindings::PlayEventOnce>("PlayEventOnce", "Play FMOD event once, returns an event instance");
    wren_class.funcExt<bindings::PlayEventLoop>("PlayEventLoop", "Play FMOD event on loop, returns an event instance");
    wren_class.funcExt<bindings::StopEvent>("StopEvent", "Stop playing an event instance");
    wren_class.funcExt<bindings::StopSFX>("StopSFX", "Stop playing a sound effect instance");
    wren_class.funcExt<&bindings::EnableLowPass>("EnableLowPass", "Enables the lowpass DSP");
    wren_class.funcExt<&bindings::DisableLowPass>("DisableLowPass", "Disables the lowpass DSP");
    wren_class.funcExt<&bindings::SetMasterChannelGroupPitch>("SetPlaybackSpeed", "Sets the playback speed of all audio using pitch");
    wren_class.func<&AudioModule::SetEventFloatAttribute>("SetEventFloatAttribute", "Pass event instance, attribute name and float value");
    wren_class.func<&AudioModule::SetEventVolume>("SetEventVolume", "Set event volume from 0.0 to 1.0");

    module.klass<WrenComponent<AudioListenerComponent>>("AudioListenerComponent");
    auto& audioEmitterComponentClass = module.klass<WrenComponent<AudioEmitterComponent>>("AudioEmitterComponent");
    audioEmitterComponentClass.funcExt<bindings::AddSFX>("AddSFX", "Add a 3D sound instance to an entity, so that it can play in 3D");
    audioEmitterComponentClass.funcExt<bindings::AddEvent>("AddEvent", "Add an event instance to an entity, so that it can play in 3D");

    module.klass<SoundInstance>("SoundInstance", "Handle to a currently playing sound");
    module.klass<EventInstance>("EventInstance", "Handle to a currently playing FMOD event");
}