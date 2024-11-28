#pragma once

#include "module_interface.hpp"

struct FMOD_SYSTEM;
struct FMOD_STUDIO_SYSTEM;
struct FMOD_SOUND;
struct FMOD_STUDIO_BANK;
struct FMOD_STUDIO_EVENTINSTANCE;
struct FMOD_CHANNELGROUP;
struct FMOD_CHANNEL;

struct SoundInfo
{
    std::string_view path;
    uint32_t uid = 0;

    bool isLoop = false;
    bool id3D = false;
    float volume = 1.0f;
};

struct BankInfo
{
    std::string_view path;
    uint32_t uid = 0;
};

class AudioModule final : public ModuleInterface
{
    ModuleTickOrder Init(Engine& engine) override;
    void Shutdown(Engine& engine) override;
    void Tick(Engine& engine) override;

public:
    AudioModule() = default;
    ~AudioModule() override = default;

    // Load sound

    void LoadSound(SoundInfo& soundInfo);

    // Play sound
    // PlaySound is already used by a MinGW macro 💀
    void PlaySoundA(SoundInfo& soundInfo);

    void StopSound(const SoundInfo& soundInfo);

    void LoadBank(BankInfo& bankInfo);

    void UnloadBank(const BankInfo& bankInfo);

    // Play an event once
    // Events started through this will stop on their own
    uint32_t StartOneShotEvent(std::string_view name);

    // Store the returned id and later call StopEvent(id), it might not stop otherwise
    NO_DISCARD uint32_t StartLoopingEvent(std::string_view name);

    void StopEvent(uint32_t eventId);

private:
    NO_DISCARD uint32_t StartEvent(std::string_view name, bool isOneShot);

    FMOD_SYSTEM* _coreSystem = nullptr;
    FMOD_STUDIO_SYSTEM* _studioSystem = nullptr;

    static constexpr uint32_t MAX_CHANNELS = 1024;

    // All sounds go through this eventually
    FMOD_CHANNELGROUP* _masterGroup = nullptr;

    // Sounds stored in FMOD
    std::unordered_map<uint32_t, FMOD_SOUND*> _sounds;
    std::unordered_map<uint32_t, FMOD_STUDIO_BANK*> _banks;
    std::unordered_map<uint32_t, FMOD_STUDIO_EVENTINSTANCE*> _events;

    // Channels that are currently active
    std::unordered_map<uint32_t, FMOD_CHANNEL*> _channelsLooping;

    uint32_t _nextEventId = 0;
};