#pragma once

#include <vector>
#include <cassert>
#include <cstdint>
#include <glm/glm.hpp>

template <typename T>
struct AnimationSpline
{

    T Sample(float time)
    {
        assert(!timestamps.empty() && "No timestamps to sample from!");

        if(time <= timestamps.front())
        {
            return values.front();
        }

        if(time >= timestamps.back())
        {
            return values.back();
        }

        for(uint32_t i = 1; i < timestamps.size(); ++i)
        {
            if(time <= timestamps[i])
            {
                float t = (time - timestamps[i - 1]) / (timestamps[i] - timestamps[i - 1]);
                return glm::mix(values[i], values[i + 1], t);
            }
        }
    }

    std::vector<float> timestamps;
    std::vector<T> values;
};