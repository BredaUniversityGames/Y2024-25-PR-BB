#pragma once

#include <string_view>

struct FMOD_SYSTEM;
struct FMOD_STUDIO_SYSTEM;
struct FMOD_SOUND;
struct FMOD_STUDIO_BANK;
struct FMOD_STUDIO_EVENTINSTANCE;
struct FMOD_CHANNELGROUP;
struct FMOD_CHANNEL;

using AudioUID = uint32_t;
using ChannelID = uint32_t;

struct SoundInfo
{
    std::string_view path {};
    AudioUID uid = 0;

    float volume = 1.0f;
    bool isLoop = false;
    bool is3D = false;
};

struct BankInfo
{
    std::string_view path {};
    AudioUID uid = 0;
};