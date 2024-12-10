#pragma once

#include <string_view>

struct FMOD_SYSTEM;
struct FMOD_STUDIO_SYSTEM;
struct FMOD_SOUND;
struct FMOD_STUDIO_BANK;
struct FMOD_STUDIO_EVENTINSTANCE;
struct FMOD_CHANNELGROUP;
struct FMOD_CHANNEL;
struct FMOD_DSP;

using AudioUID = int32_t;
using ChannelID = int32_t;

struct SoundInfo
{
    std::string_view path {};
    AudioUID uid = -1;

    bool isLoop = false;
    bool is3D = false;
};

struct SoundInstance
{
    SoundInstance();
    explicit SoundInstance(const ChannelID channelId)
        : id(channelId)
    {
    }
    ChannelID id = -1;
};

struct BankInfo
{
    std::string_view path {};
    AudioUID uid = -1;
};