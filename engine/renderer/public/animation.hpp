#pragma once

#include "imgui_entt_entity_editor.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>
#include <vector>

using Translation = glm::vec3;
using Rotation = glm::quat;
using Scale = glm::vec3;

template <typename T>
inline T interpolate(const T& a, const T& b, float t);

template <>
inline glm::vec3 interpolate<glm::vec3>(const glm::vec3& a, const glm::vec3& b, float t)
{
    return glm::mix(a, b, t);
}

template <>
inline glm::quat interpolate<glm::quat>(const glm::quat& a, const glm::quat& b, float t)
{
    return glm::slerp(a, b, t);
}

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
        return interpolate(values[i - 1], values[i], t);
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
    PlaybackOptions playbackOption { Animation::PlaybackOptions::eStopped };
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
            time = std::fmod(time, duration);
        }
    }

    float ScaledDuration() const
    {
        return duration / speed;
    }
    float ScaledTime() const
    {
        return time / speed;
    }
};

struct AnimationControlComponent
{
    std::vector<Animation> animations;
    std::optional<uint32_t> activeAnimation { std::nullopt };
    std::optional<uint32_t> transitionAnimation { std::nullopt };
    float blendTime;
    float remainingBlendTime;

    void PlayByIndex(uint32_t animationIndex, float speed = 1.0f, bool looping = false, float blendTime = 0.0f, bool blendMatch = false);
    void Play(const std::string& name, float speed = 1.0f, bool looping = false, float blendTime = 0.0f, bool blendMatch = false);
    void Stop();
    void Pause();
    void Resume();
    Animation::PlaybackOptions CurrentPlayback();
    std::optional<std::string> CurrentAnimationName();
    std::optional<uint32_t> CurrentAnimationIndex();
    bool AnimationFinished();
};
namespace EnttEditor
{
template <>
void ComponentEditorWidget<AnimationControlComponent>(entt::registry& reg, entt::registry::entity_type e);
}
