#pragma once

#include "audio_common.hpp"

struct FMOD_CHANNEL;
struct AudioEmitterComponent
{
    std::string_view _soundOrEventName = "assets/fallback.mp3";
    std::vector<AudioUID> ids {};

    bool _playFlag = false;

    void Play(std::string_view path, bool loop, bool is3D);
};