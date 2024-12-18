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
    std::string name { "" };
    float duration { 0.0f };
    float time { 0.0f };

    void Update(float dt, uint32_t frameIndex)
    {
        if (_frameIndex != frameIndex)
        {
            _frameIndex = frameIndex;
            time += dt;

            if (time > duration)
            {
                time = 0.0f;
            }
        }
    }

private:
    uint32_t _frameIndex { 0 };
};

struct AnimationControlComponent
{
    std::vector<Animation> animations;
    std::optional<uint32_t> activeAnimation { std::nullopt };
};
