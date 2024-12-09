#pragma once

#include <memory>
#include <optional>

#include "animation.hpp"

struct AnimationChannelComponent
{
    std::shared_ptr<Animation> animation { nullptr };

    std::optional<AnimationSpline<Translation>> translation { std::nullopt };
    std::optional<AnimationSpline<Rotation>> rotation { std::nullopt };
    std::optional<AnimationSpline<Scale>> scaling { std::nullopt };
};
