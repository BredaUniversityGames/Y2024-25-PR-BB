#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "animation.hpp"

struct TransformAnimationSpline
{
    std::optional<AnimationSpline<Translation>> translation { std::nullopt };
    std::optional<AnimationSpline<Rotation>> rotation { std::nullopt };
    std::optional<AnimationSpline<Scale>> scaling { std::nullopt };
};

struct AnimationChannelComponent
{
    entt::entity animationControlEntity { entt::null };
    // Keys are based on the animation index from the AnimationControl.
    std::unordered_map<uint32_t, TransformAnimationSpline> animationSplines;
};

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AnimationChannelComponent>(entt::registry& reg, entt::registry::entity_type e);
}
