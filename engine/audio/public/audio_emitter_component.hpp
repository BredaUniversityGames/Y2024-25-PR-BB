#pragma once

#include "audio_common.hpp"

struct FMOD_CHANNEL;
struct AudioEmitterComponent
{
    std::vector<AudioUID> ids {};

    bool _playFlag = false;
};