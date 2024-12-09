#pragma once

#include "audio_common.hpp"
#include "common.hpp"
#include "module_interface.hpp"
#include <glm/glm.hpp>
#include <string_view>
#include <unordered_map>

class AudioModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

public:
    AudioModule() = default;
    ~AudioModule() override = default;

    // Load sound, mp3 or .wav etc
    void LoadSFX(SoundInfo& soundInfo);

    // Play sound
    // PlaySound(...) is already used by a MinGW macro 💀
    void PlaySFX(SoundInfo& soundInfo);

    // Stops looping sounds
    // Regular sounds will stop by themselves once they are done
    void StopSFX(const SoundInfo& soundInfo);

    // Load a .bank file
    // make sure to load the master bank and .strings.bank as well
    void LoadBank(BankInfo& bankInfo);

    // Unload a bank
    void UnloadBank(const BankInfo& bankInfo);

    // Play an event once
    // Events started through this will stop on their own
    AudioUID StartOneShotEvent(std::string_view name);

    // Start an event that should play at least once
    // Store the returned id and later call StopEvent(id), it might not stop otherwise
    NO_DISCARD AudioUID StartLoopingEvent(std::string_view name);

    // Stops an event that is
    void StopEvent(AudioUID eventId);

    void SetListener3DAttributes(const glm::vec3& position) const;

    void Update3DSoundPosition(const AudioUID id, const glm::vec3& position);

private:
    NO_DISCARD AudioUID
    StartEvent(std::string_view name, bool isOneShot);

    FMOD_SYSTEM* _coreSystem = nullptr;
    FMOD_STUDIO_SYSTEM* _studioSystem = nullptr;

    static constexpr uint32_t MAX_CHANNELS = 1024;

    // All sounds go through this eventually
    FMOD_CHANNELGROUP* _masterGroup = nullptr;

    std::unordered_map<AudioUID, FMOD_SOUND*> _sounds {};
    std::unordered_map<AudioUID, FMOD_STUDIO_BANK*> _banks {};
    std::unordered_map<AudioUID, FMOD_STUDIO_EVENTINSTANCE*> _events {};

    std::unordered_map<AudioUID, FMOD_CHANNEL*> _channelsActive {};

    AudioUID _nextEventId = 0;
};