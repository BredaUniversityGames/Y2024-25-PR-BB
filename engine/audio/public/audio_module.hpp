#pragma once

#include "audio_common.hpp"
#include "common.hpp"
#include "module_interface.hpp"
#include <glm/glm.hpp>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class PhysicsModule;

class AudioModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

    std::string_view GetName() override { return "Audio Module"; }

public:
    AudioModule() = default;
    ~AudioModule() override = default;

    // Resets all playing audio
    void Reset();

    // Load sound, mp3 or .wav etc
    SoundID LoadSFX(SoundInfo& soundInfo);

    // Checks if the sound is loaded
    bool isSFXLoaded(std::string_view path) const;

    // Return the soundinfo associated with the path
    SoundID GetSFX(std::string_view path);

    // Play sound
    // PlaySound(...) is already used by a MinGW macro 💀
    SoundInstance PlaySFX(SoundID, float volume, bool startPaused);

    // Set paused or unpaused
    void SetPaused(ChannelID instance, bool paused);

    // Stops looping sounds
    // Regular sounds will stop by themselves once they are done
    void StopSFX(SoundInstance instance);

    bool IsSFXPlaying(SoundInstance instance);

    // Load a .bank file
    // make sure to load the master bank and .strings.bank as well
    void LoadBank(std::string_view path);

    // Unload a bank
    void UnloadBank(const BankInfo& bankInfo);

    // Play an event once
    // Events started through this will stop on their own
    EventInstance StartOneShotEvent(std::string_view name);

    // Start an event that should play at least once
    // Store the returned id and later call StopEvent(id), it will not stop otherwise
    NO_DISCARD EventInstance StartLoopingEvent(std::string_view name);

    // Stops an event that is
    void StopEvent(EventInstance instance);

    bool IsEventPlaying(EventInstance instance);

    // Set volume of an event, from 0.0 to 1.0
    void SetEventVolume(EventInstance ev, float volume);

    void SetEventFloatAttribute(EventInstance ev, const std::string& name, float val);

    void SetListener3DAttributes(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up) const;

    void UpdateSound3DAttributes(ChannelID id, const glm::vec3& position, const glm::vec3& velocity);

    void SetEvent3DAttributes(EventInstance id, const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up);

    void RegisterChannelBus(const std::string& busName);
    void SetBusChannelVolume(const std::string& busName, float scale);

    void SetLowpassBypass(bool state);

    void SetMasterChannelGroupPitch(float pitch);

    std::vector<glm::vec3>& GetDebugLines()
    {
        return _debugLines;
    }

    void AddDebugLine(const glm::vec3& start, const glm::vec3& end)
    {
        _debugLines.emplace_back(start);
        _debugLines.emplace_back(end);
    }

    void ClearLines()
    {
        _debugLines.clear();
    }

private:
    friend class AudioSystem;
    NO_DISCARD EventInstance StartEvent(std::string_view name, bool isOneShot);

    // This function is intended to be used bby the audio system only
    void StartEventInstance(EventInstance instance);

    FMOD_SYSTEM* _coreSystem = nullptr;
    FMOD_STUDIO_SYSTEM* _studioSystem = nullptr;

    std::unordered_map<std::string, FMOD_STUDIO_BUS*> _eventBusMap;

    static constexpr uint32_t MAX_CHANNELS = 1024;

    // All sounds go through this eventually
    // USED ONLY FOR DSP VISUALIZATION: for events, we use event buses
    FMOD_CHANNELGROUP* _masterGroup = nullptr;
    FMOD_DSP* _lowPassDSP = nullptr;
    FMOD_DSP* _fftDSP = nullptr;

    std::unordered_map<std::string, SoundInfo> _soundInfos {};

    std::unordered_map<SoundID, FMOD_SOUND*> _sounds {};
    std::unordered_map<BankID, FMOD_STUDIO_BANK*> _banks {};
    std::unordered_map<EventInstanceID, FMOD_STUDIO_EVENTINSTANCE*> _events {};

    std::unordered_map<ChannelID, FMOD_CHANNEL*> _channelsActive {};

    std::queue<ChannelID> soundsToPlay {};

    EventInstanceID _nextEventId = 0;
    SoundID _nextSoundId = 0;

    PhysicsModule* _physics = nullptr;

    // Debug lines
    std::vector<glm::vec3> _debugLines {};
};