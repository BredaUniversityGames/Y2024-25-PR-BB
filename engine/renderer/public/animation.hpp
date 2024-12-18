#pragma once

#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

using Translation = glm::vec3;
using Rotation = glm::quat;
using Scale = glm::vec3;

template <typename T>
struct AnimationSpline
{
    T Sample(float time)
    {
        auto it = std::lower_bound(timestamps.begin(), timestamps.end(), time);

        if (it == timestamps.begin())
        {
            return values.front();
        }

        if (it == timestamps.end())
        {
            return values.back();
        }

        uint32_t i = it - timestamps.begin();

        float t = (time - timestamps[i - 1]) / (timestamps[i] - timestamps[i - 1]);

        return glm::mix(values[i - 1], values[i], t);
    }

    std::vector<float> timestamps;
    std::vector<T> values;
};

struct Animation
{
    enum class PlaybackOptions
    {
        ePlaying,
        ePaused,
        eStopped,
    };

    std::string name { "" };
    float duration { 0.0f };
    float time { 0.0f };
    float speed { 1.0f };
    PlaybackOptions playbackOption;
    bool looping = false;

    void Update(float dt)
    {
        switch (playbackOption)
        {
        case PlaybackOptions::ePlaying:
            time += dt * speed;
            break;
        case PlaybackOptions::ePaused:
            // Do nothing.
            break;
        case PlaybackOptions::eStopped:
            time = 0.0f;
            break;
        }

        if (time > duration && looping)
        {
            time = 0.0f;
        }
    }
};

struct AnimationControlComponent
{
    std::vector<Animation> animations;
    std::optional<uint32_t> activeAnimation { std::nullopt };
};
