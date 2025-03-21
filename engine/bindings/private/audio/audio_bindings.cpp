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
    auto& wren_class = module.klass<AudioModule>("Audio");
    wren_class.funcExt<bindings::LoadBank>("LoadBank");
    wren_class.funcExt<bindings::LoadSFX>("LoadSFX");
    wren_class.funcExt<bindings::PlaySFX>("PlaySFX");
    wren_class.funcExt<bindings::IsSFXPlaying>("IsSFXPlaying");
    wren_class.funcExt<bindings::PlayEventOnce>("PlayEventOnce");
    wren_class.funcExt<bindings::PlayEventLoop>("PlayEventLoop");
    wren_class.funcExt<bindings::StopEvent>("StopEvent");

    module.klass<WrenComponent<AudioListenerComponent>>("AudioListenerComponent");
    auto& audioEmitterComponentClass = module.klass<WrenComponent<AudioEmitterComponent>>("AudioEmitterComponent");
    audioEmitterComponentClass.funcExt<bindings::AddSFX>("AddSFX");
    audioEmitterComponentClass.funcExt<bindings::AddEvent>("AddEvent");

    module.klass<SoundInstance>("SoundInstance");
    module.klass<EventInstance>("EventInstance");
}