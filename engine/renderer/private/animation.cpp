#include "animation.hpp"

#include <algorithm>

#include "log.hpp"

void AnimationControlComponent::PlayByIndex(uint32_t animationIndex, float speed, bool looping, float blendTime, bool blendMatch)
{
    bool useBlend = blendTime > 0.0f && activeAnimation.has_value();
    if (useBlend)
    {
        transitionAnimation = activeAnimation;

        this->blendTime = blendTime;
        this->remainingBlendTime = blendTime;
    }

    activeAnimation = animationIndex;
    animations[animationIndex].time = useBlend && blendMatch ? animations[transitionAnimation.value()].time * (animations[animationIndex].duration / animations[transitionAnimation.value()].duration) : 0.0f;
    animations[animationIndex].looping = looping;
    animations[animationIndex].speed = speed;
    animations[animationIndex].playbackOption = Animation::PlaybackOptions::ePlaying;
}

void AnimationControlComponent::Play(const std::string& name, float speed, bool looping, float blendTime, bool blendMatch)
{
    auto it = std::find_if(animations.begin(), animations.end(), [&name](const auto& anim)
        { return anim.name == name; });

    if (it != animations.end())
    {
        PlayByIndex(std::distance(animations.begin(), it), speed, looping, blendTime, blendMatch);
    }
    else
    {
        bblog::warn("Tried to use invalid animation name: {}", name);
    }
}

void AnimationControlComponent::Stop()
{
    if (!activeAnimation.has_value())
    {
        return;
    }
    Animation& animation = animations[activeAnimation.value()];
    animation.time = 0.0f;
    animation.playbackOption = Animation::PlaybackOptions::eStopped;
}

void AnimationControlComponent::Pause()
{
    if (!activeAnimation.has_value())
    {
        return;
    }
    Animation& animation = animations[activeAnimation.value()];
    animation.playbackOption = Animation::PlaybackOptions::ePaused;
}

void AnimationControlComponent::Resume()
{
    if (!activeAnimation.has_value())
    {
        return;
    }
    Animation& animation = animations[activeAnimation.value()];
    animation.playbackOption = Animation::PlaybackOptions::ePlaying;
}

Animation::PlaybackOptions AnimationControlComponent::CurrentPlayback()
{
    if (!activeAnimation.has_value())
    {
        return Animation::PlaybackOptions::eStopped;
    }

    return animations[activeAnimation.value()].playbackOption;
}

std::optional<std::string> AnimationControlComponent::CurrentAnimationName()
{
    if (!activeAnimation.has_value())
    {
        return std::nullopt;
    }

    return animations[activeAnimation.value()].name;
}

std::optional<uint32_t> AnimationControlComponent::CurrentAnimationIndex()
{
    return activeAnimation;
}

bool AnimationControlComponent::AnimationFinished()
{
    if (!activeAnimation.has_value())
    {
        return true;
    }

    return animations[activeAnimation.value()].time > animations[activeAnimation.value()].duration || animations[activeAnimation.value()].playbackOption == Animation::PlaybackOptions::eStopped;
}

namespace EnttEditor
{
template <>
void ComponentEditorWidget<AnimationControlComponent>(entt::registry& reg, entt::registry::entity_type e)
{
    auto& comp = reg.get<AnimationControlComponent>(e);
    // ImGui::SliderFloat("Blend Ratio##AnimationControl", &comp.blendRatio, 0.0, 1.0);

    ImGui::LabelText("Blend time", "%f", comp.blendTime);
    ImGui::LabelText("Remaining blend time", "%f", comp.remainingBlendTime);

    if (comp.activeAnimation.has_value())
    {
        const Animation& activeAnimation = comp.animations[comp.activeAnimation.value()];
        ImGui::LabelText("Active animation", "%s", activeAnimation.name.c_str());
        ImGui::LabelText("Time", "%f", activeAnimation.time);
        ImGui::LabelText("Duration", "%f", activeAnimation.duration);
    }
    if (comp.transitionAnimation.has_value())
    {
        const Animation& transitionAnimation = comp.animations[comp.transitionAnimation.value()];
        ImGui::LabelText("Active animation", "%s", transitionAnimation.name.c_str());
        ImGui::LabelText("Time", "%f", transitionAnimation.time);
        ImGui::LabelText("Duration", "%f", transitionAnimation.duration);
    }
}
}
