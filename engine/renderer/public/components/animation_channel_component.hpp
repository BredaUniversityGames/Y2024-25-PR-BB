#pragma once

#include <memory>
#include <optional>

#include "animation.hpp"

struct TransformAnimationSpline
{
    std::optional<AnimationSpline<Translation>> translation { std::nullopt };
    std::optional<AnimationSpline<Rotation>> rotation { std::nullopt };
    std::optional<AnimationSpline<Scale>> scaling { std::nullopt };
};

struct AnimationChannelComponent
{
    AnimationControlComponent* animationControl { nullptr };
    // Keys are based on the animation index from the AnimationControl.
    std::unordered_map<uint32_t, TransformAnimationSpline> animationSplines;
};
