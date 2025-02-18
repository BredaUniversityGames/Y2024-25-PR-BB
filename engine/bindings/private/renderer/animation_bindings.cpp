#include "animation_bindings.hpp"

#include "animation.hpp"
#include "entity/wren_entity.hpp"
#include "utility/enum_bind.hpp"

namespace bindings
{
int32_t AnimationControlComponentGetAnimationCount(WrenComponent<AnimationControlComponent>& component)
{
    return component.component->animations.size();
}
void AnimationControlComponentPlay(WrenComponent<AnimationControlComponent>& component, const std::string& name, float speed, bool looping)
{
    component.component->Play(name, speed, looping);
}
void AnimationControlComponentStop(WrenComponent<AnimationControlComponent>& component)
{
    component.component->Stop();
}
void AnimationControlComponentPause(WrenComponent<AnimationControlComponent>& component)
{
    component.component->Pause();
}
void AnimationControlComponentResume(WrenComponent<AnimationControlComponent>& component)
{
    component.component->Resume();
}
Animation::PlaybackOptions AnimationControlComponentCurrentPlayback(WrenComponent<AnimationControlComponent>& component)
{
    return component.component->CurrentPlayback();
}
std::optional<uint32_t> AnimationControlComponentCurrentAnimationIndex(WrenComponent<AnimationControlComponent>& component)
{
    return component.component->CurrentAnimationIndex();
}
std::optional<std::string> AnimationControlComponentCurrentAnimationName(WrenComponent<AnimationControlComponent>& component)
{
    return component.component->CurrentAnimationName();
}
bool AnimationControlComponentAnimationFinished(WrenComponent<AnimationControlComponent>& component)
{
    return component.component->AnimationFinished();
}
}

void BindAnimationAPI(wren::ForeignModule& module)
{
    bindings::BindEnum<Animation::PlaybackOptions>(module, "PlaybackOptions");

    auto& animationControlClass = module.klass<WrenComponent<AnimationControlComponent>>("AnimationControlComponent");
    animationControlClass.funcExt<bindings::AnimationControlComponentGetAnimationCount>("GetAnimationCount");
    animationControlClass.funcExt<bindings::AnimationControlComponentPlay>("Play");
    animationControlClass.funcExt<bindings::AnimationControlComponentStop>("Stop");
    animationControlClass.funcExt<bindings::AnimationControlComponentPause>("Pause");
    animationControlClass.funcExt<bindings::AnimationControlComponentResume>("Resume");
    animationControlClass.funcExt<bindings::AnimationControlComponentCurrentPlayback>("CurrentPlayback");
    animationControlClass.funcExt<bindings::AnimationControlComponentCurrentAnimationIndex>("CurrentAnimationIndex");
    animationControlClass.funcExt<bindings::AnimationControlComponentCurrentAnimationName>("CurrentAnimationName");
    animationControlClass.funcExt<bindings::AnimationControlComponentAnimationFinished>("AnimationFinished");
}