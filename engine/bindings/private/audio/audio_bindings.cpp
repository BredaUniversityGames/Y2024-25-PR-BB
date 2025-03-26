#include "audio_bindings.hpp"

#include "audio_emitter_component.hpp"
#include "audio_listener_component.hpp"
#include "audio_module.hpp"
#include "entity/wren_entity.hpp"
#include "log.hpp"

namespace bindings
{
void LoadBank(AudioModule& self, const std::string& path)
{
    BankInfo bi {};
    bi.path = path;

    self.LoadBank(bi);
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

bool IsSFXPlaying(AudioModule& self, const SoundInstance instance)
{
    return self.IsSFXPlaying(instance);
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

void AddSFX(WrenComponent<AudioEmitterComponent>& self, SoundInstance& instance)
{
    self.component->_soundIds.emplace_back(instance);
}

void AddEvent(WrenComponent<AudioEmitterComponent>& self, EventInstance& instance)
{
    self.component->_eventIds.emplace_back(instance);
}
}

void BindAudioAPI(wren::ForeignModule& module)
{
    auto& wren_class = module.klass<AudioModule>("Audio", "Manages Audio 2D and 3D audio output and loading");
    wren_class.funcExt<bindings::LoadBank>("LoadBank", "Loads a FMOD audio bank internally with path provided");
    wren_class.funcExt<bindings::LoadSFX>("LoadSFX", "Loads an audio file internally with path provided, bool is3D and bool isLooping");
    wren_class.funcExt<bindings::PlaySFX>("PlaySFX", "Plays a previously loaded sound with provided path and volume, returns a sound instance");
    wren_class.funcExt<bindings::IsSFXPlaying>("IsSFXPlaying", "Checks if a specific sound instance is still playing");
    wren_class.funcExt<bindings::PlayEventOnce>("PlayEventOnce", "Play FMOD event once, returns an event instance");
    wren_class.funcExt<bindings::PlayEventLoop>("PlayEventLoop", "Play FMOD event on loop, returns an event instance");
    wren_class.funcExt<bindings::StopEvent>("StopEvent", "Stop playing an event instance");

    module.klass<WrenComponent<AudioListenerComponent>>("AudioListenerComponent");
    auto& audioEmitterComponentClass = module.klass<WrenComponent<AudioEmitterComponent>>("AudioEmitterComponent");
    audioEmitterComponentClass.funcExt<bindings::AddSFX>("AddSFX", "Add a 3D sound instance to an entity, so that it can play in 3D");
    audioEmitterComponentClass.funcExt<bindings::AddEvent>("AddEvent", "Add an event instance to an entity, so that it can play in 3D");

    module.klass<SoundInstance>("SoundInstance", "Handle to a currently playing sound");
    module.klass<EventInstance>("EventInstance", "Handle to a currently playing FMOD event");
}