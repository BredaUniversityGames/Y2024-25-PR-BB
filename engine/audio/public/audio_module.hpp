#pragma once

#include "module_interface.hpp"

class FMOD_SYSTEM;
class FMOD_STUDIO_SYSTEM;
class FMOD_SOUND;
class FMOD_CHANNELGROUP;
class FMOD_CHANNEL;

struct SoundInfo
{
    std::string_view path;
    uint32_t uid = 0;

    bool isLoop = false;
    bool id3D = false;
    float volume = 1.0f;

    bool isLoaded = false;
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

private:
    FMOD_SYSTEM* _coreSystem = nullptr;
    FMOD_STUDIO_SYSTEM* _studioSystem = nullptr;

    static constexpr uint32_t MAX_CHANNELS = 1024;

    // All sounds go through this eventually
    FMOD_CHANNELGROUP* _masterGroup = nullptr;

    // Sounds stored in FMOD
    std::unordered_map<uint32_t, FMOD_SOUND*> _sounds;

    // Channels that are currently active
    std::unordered_map<uint32_t, FMOD_CHANNEL*> _channelsLooping;
};