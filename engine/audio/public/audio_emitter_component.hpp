#pragma once

#include "audio_common.hpp"

struct FMOD_CHANNEL;
struct AudioEmitterComponent
{
    std::vector<SoundInstance> ids {};

    bool _playFlag = false;
};