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

using BaseID = size_t;

// using AudioID = BaseID;
using ChannelID = BaseID;
using BankID = BaseID;
using EventInstanceID = BaseID;
using SoundID = BaseID;

struct SoundInfo
{
    SoundInfo() = default;
    std::string_view path {};
    SoundID uid = -1;

    bool isLoop = false;
    bool is3D = false;
};

struct SoundInstance
{
    SoundInstance();
    explicit SoundInstance(const ChannelID channelId, const bool isSound3D)
        : id(channelId)
        , is3D(isSound3D)
    {
    }
    ChannelID id = -1;
    bool is3D;
};

struct BankInfo
{
    std::string_view path {};
    BankID uid = -1;
};