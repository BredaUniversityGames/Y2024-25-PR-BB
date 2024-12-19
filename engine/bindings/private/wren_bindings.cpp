#include "wren_bindings.hpp"

#include "utility/enum_bind.hpp"
#include "utility/wren_entity.hpp"
#include "wren_engine.hpp"

#include "ecs_module.hpp"
#include "time_module.hpp"

#include "animation.hpp"
#include "application_module.hpp"
#include "components/name_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "input/input_codes/keys.hpp"
#include "input/input_codes/mousebuttons.hpp"

#include <cstdint>
#include <input/action_manager.hpp>

namespace bindings
{
void BindMath(wren::ForeignModule& module);
void BindMathHelper(wren::ForeignModule& module);
void BindEntity(wren::ForeignModule& module);

float TimeModuleGetDeltatime(TimeModule& self)
{
    return self.GetDeltatime().count();
}

WrenEntity CreateEntity(ECSModule& self)
{
    return { self.GetRegistry().create(), &self.GetRegistry() };
}

void FreeEntity(ECSModule& self, WrenEntity& entity)
{
    self.GetRegistry().destroy(entity.entity);
}

std::optional<WrenEntity> GetEntityByName(ECSModule& self, const std::string& name)
{
    auto view = self.GetRegistry().view<NameComponent>();
    for (auto&& [e, n] : view.each())
    {
        if (n.name == name)
        {
            return WrenEntity { e, &self.GetRegistry() };
        }
    }

    return std::nullopt;
}

bool InputGetDigitalAction(ApplicationModule& self, const std::string& action_name)
{
    return self.GetActionManager().GetDigitalAction(action_name);
}

glm::vec2 InputGetAnalogAction(ApplicationModule& self, const std::string& action_name)
{
    return self.GetActionManager().GetAnalogAction(action_name);
}

bool InputGetRawKeyOnce(ApplicationModule& self, KeyboardCode code)
{
    return self.GetInputDeviceManager().IsKeyPressed(code);
}

glm::vec3 TransformComponentGetTranslation(WrenComponent<TransformComponent>& component)
{
    return component.component->GetLocalPosition();
}

glm::quat TransformComponentGetRotation(WrenComponent<TransformComponent>& component)
{
    return component.component->GetLocalRotation();
}

glm::vec3 TransformComponentGetScale(WrenComponent<TransformComponent>& component)
{
    return component.component->GetLocalScale();
}

void TransformComponentSetTranslation(WrenComponent<TransformComponent>& component, const glm::vec3& translation)
{
    TransformHelpers::SetLocalPosition(*component.entity.registry, component.entity.entity, translation);
}

void TransformComponentSetRotation(WrenComponent<TransformComponent>& component, const glm::quat& rotation)
{
    TransformHelpers::SetLocalRotation(*component.entity.registry, component.entity.entity, rotation);
}

void TransformComponentSetScale(WrenComponent<TransformComponent>& component, const glm::vec3& scale)
{
    TransformHelpers::SetLocalScale(*component.entity.registry, component.entity.entity, scale);
}

std::string NameComponentGetName(WrenComponent<NameComponent>& nameComponent)
{
    return nameComponent.component->name;
}

int32_t AnimationControlComponentGetAnimationCount(WrenComponent<AnimationControlComponent>& component)
{
    return component.component->animations.size();
}
void AnimationControlComponentPlay(WrenComponent<AnimationControlComponent>& component, const std::string& name, float speed, bool looping)
{
    component.component->Play(name, speed, looping);
}
void AnimationControlComponentPlayByIndex(WrenComponent<AnimationControlComponent>& component, uint32_t index, float speed, bool looping)
{
    component.component->PlayByIndex(index, speed, looping);
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

void BindEngineAPI(wren::ForeignModule& module)
{
    bindings::BindMath(module);
    bindings::BindMathHelper(module);
    bindings::BindEntity(module);

    // Add modules here to expose them in scripting
    {
        auto& engineAPI = module.klass<WrenEngine>("Engine");
        engineAPI.func<&WrenEngine::GetModule<TimeModule>>("GetTime");
        engineAPI.func<&WrenEngine::GetModule<ECSModule>>("GetECS");
        engineAPI.func<&WrenEngine::GetModule<ApplicationModule>>("GetInput");
    }

    // Time Module
    {
        auto& time = module.klass<TimeModule>("TimeModule");
        time.funcExt<bindings::TimeModuleGetDeltatime>("GetDeltatime");
    }

    // ECS module
    {
        // ECS class
        auto& wrenClass = module.klass<ECSModule>("ECS");
        wrenClass.funcExt<bindings::CreateEntity>("NewEntity");
        wrenClass.funcExt<bindings::GetEntityByName>("GetEntityByName");
        wrenClass.funcExt<bindings::FreeEntity>("DestroyEntity");
    }

    // Input
    {
        auto& wrenClass = module.klass<ApplicationModule>("Input");
        wrenClass.funcExt<bindings::InputGetDigitalAction>("GetDigitalAction");
        wrenClass.funcExt<bindings::InputGetAnalogAction>("GetAnalogAction");
        wrenClass.funcExt<bindings::InputGetRawKeyOnce>("DebugGetKey");

        bindings::BindEnum<KeyboardCode>(module, "Keycode");
    }

    // Components
    {
        // Name class
        auto& nameClass = module.klass<WrenComponent<NameComponent>>("NameComponent");
        nameClass.propReadonlyExt<bindings::NameComponentGetName>("name");

        // Transform component
        auto& transformClass = module.klass<WrenComponent<TransformComponent>>("TransformComponent");

        transformClass.propExt<
            bindings::TransformComponentGetTranslation, bindings::TransformComponentSetTranslation>("translation");

        transformClass.propExt<
            bindings::TransformComponentGetRotation, bindings::TransformComponentSetRotation>("rotation");

        transformClass.propExt<
            bindings::TransformComponentGetScale, bindings::TransformComponentSetScale>("scale");

        bindings::BindEnum<Animation::PlaybackOptions>(module, "PlaybackOptions");

        auto& animationControlClass = module.klass<WrenComponent<AnimationControlComponent>>("AnimationControlComponent");
        animationControlClass.funcExt<bindings::AnimationControlComponentGetAnimationCount>("GetAnimationCount");
        animationControlClass.funcExt<bindings::AnimationControlComponentPlay>("Play");
        animationControlClass.funcExt<bindings::AnimationControlComponentPlayByIndex>("PlayByIndex");
        animationControlClass.funcExt<bindings::AnimationControlComponentStop>("Stop");
        animationControlClass.funcExt<bindings::AnimationControlComponentPause>("Pause");
        animationControlClass.funcExt<bindings::AnimationControlComponentResume>("Resume");
        animationControlClass.funcExt<bindings::AnimationControlComponentCurrentPlayback>("CurrentPlayback");
        animationControlClass.funcExt<bindings::AnimationControlComponentCurrentAnimationIndex>("CurrentAnimationIndex");
        animationControlClass.funcExt<bindings::AnimationControlComponentCurrentAnimationName>("CurrentAnimationName");
        animationControlClass.funcExt<bindings::AnimationControlComponentAnimationFinished>("AnimationFinished");
    }
}

// BINDING MATH TYPES

namespace vector_ops
{
template <typename T>
static T Default() { return {}; }

template <typename T>
static T Add(T& lhs, const T& rhs) { return lhs + rhs; }

template <typename T>
static T Sub(T& lhs, const T& rhs) { return lhs - rhs; }

template <typename T>
static T Neg(T& lhs) { return -lhs; }

template <typename T>
static T Mul(T& lhs, const T& rhs) { return lhs * rhs; }

template <typename T>
static bool Equals(T& lhs, const T& rhs) { return lhs == rhs; }

template <typename T>
static bool NotEquals(T& lhs, const T& rhs) { return lhs != rhs; }

template <typename T>
static T Normalized(T& v) { return glm::normalize(v); }

template <typename T>
static float Length(T& v) { return glm::length(v); }

};

class MathUtil
{
public:
    static glm::vec3 ToEuler(glm::quat quat)
    {
        return glm::eulerAngles(quat);
    }
    static glm::quat ToQuat(glm::vec3 euler)
    {
        return glm::quat { euler };
    }
    static float PI()
    {
        return glm::pi<float>();
    }
    static float TwoPI()
    {
        return glm::two_pi<float>();
    }
    static float HalfPI()
    {
        return glm::half_pi<float>();
    }
};

template <typename T>
void BindVectorTypeOperations(wren::ForeignKlassImpl<T>& klass)
{
    klass.template funcStaticExt<vector_ops::Default<T>>("Default");
    klass.template funcExt<vector_ops::Add<T>>(wren::OPERATOR_ADD);
    klass.template funcExt<vector_ops::Sub<T>>(wren::OPERATOR_SUB);
    klass.template funcExt<vector_ops::Neg<T>>(wren::OPERATOR_NEG);
    klass.template funcExt<vector_ops::Mul<T>>(wren::OPERATOR_MUL);
    klass.template funcExt<vector_ops::Equals<T>>(wren::OPERATOR_EQUAL);
    klass.template funcExt<vector_ops::NotEquals<T>>(wren::OPERATOR_NOT_EQUAL);
    klass.template funcExt<vector_ops::Normalized<T>>("normalize");
    klass.template funcExt<vector_ops::Length<T>>("length");
}

void bindings::BindMath(wren::ForeignModule& module)
{
    {
        auto& vector2 = module.klass<glm::vec2>("Vec2");
        vector2.ctor<float, float>();
        vector2.var<&glm::vec2::x>("x");
        vector2.var<&glm::vec2::y>("y");
        BindVectorTypeOperations(vector2);
    }

    {
        auto& vector3 = module.klass<glm::vec3>("Vec3");
        vector3.ctor<float, float, float>();
        vector3.var<&glm::vec3::x>("x");
        vector3.var<&glm::vec3::y>("y");
        vector3.var<&glm::vec3::z>("z");
        BindVectorTypeOperations(vector3);
    }

    {
        auto& quat = module.klass<glm::quat>("Quat");
        quat.ctor<float, float, float, float>();
        quat.var<&glm::quat::w>("w");
        quat.var<&glm::quat::x>("x");
        quat.var<&glm::quat::y>("y");
        quat.var<&glm::quat::z>("z");
        BindVectorTypeOperations(quat);
    }
}

void bindings::BindMathHelper(wren::ForeignModule& module)
{
    auto& mathUtilClass = module.klass<MathUtil>("MathUtil");
    mathUtilClass.funcStatic<&MathUtil::ToEuler>("ToEuler");
    mathUtilClass.funcStatic<&MathUtil::ToQuat>("ToQuat");
    mathUtilClass.funcStatic<&MathUtil::PI>("PI");
    mathUtilClass.funcStatic<&MathUtil::TwoPI>("TwoPI");
    mathUtilClass.funcStatic<&MathUtil::HalfPI>("HalfPI");
}

void bindings::BindEntity(wren::ForeignModule& module)
{
    // Entity class
    auto& entityClass = module.klass<WrenEntity>("Entity");

    entityClass.func<&WrenEntity::GetComponent<TransformComponent>>("GetTransformComponent");
    entityClass.func<&WrenEntity::AddComponent<TransformComponent>>("AddTransformComponent");

    entityClass.func<&WrenEntity::GetComponent<NameComponent>>("GetNameComponent");
    entityClass.func<&WrenEntity::AddComponent<NameComponent>>("AddNameComponent");

    entityClass.func<&WrenEntity::GetComponent<AnimationControlComponent>>("GetAnimationControlComponent");
}
