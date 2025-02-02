#include "wren_bindings.hpp"

#include "animation.hpp"
#include "application_module.hpp"
#include "audio/audio_bindings.hpp"
#include "audio_emitter_component.hpp"
#include "audio_module.hpp"
#include "components/name_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"
#include "ecs_module.hpp"
#include "game/game_bindings.hpp"
#include "input/input_bindings.hpp"
#include "lifetime_component.hpp"
#include "particle_module.hpp"
#include "particles/particle_bindings.hpp"
#include "physics/physics_bindings.hpp"
#include "physics_module.hpp"
#include "renderer/animation_bindings.hpp"
#include "time_module.hpp"
#include "utility/enum_bind.hpp"
#include "utility/wren_entity.hpp"
#include "wren_engine.hpp"

#include <cstdint>

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
        engineAPI.func<&WrenEngine::GetModule<AudioModule>>("GetAudio");
        engineAPI.func<&WrenEngine::GetModule<ParticleModule>>("GetParticles");
        engineAPI.func<&WrenEngine::GetModule<PhysicsModule>>("GetPhysics");
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
        BindInputAPI(module);
    }

    // Audio
    {
        BindAudioAPI(module);
    }

    // Animations
    {
        BindAnimationAPI(module);
    }

    // Particles
    {
        BindParticleAPI(module);
    }

    // Physics
    {
        BindPhysicsAPI(module);
    }

    // Game
    {
        BindGameAPI(module);
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
    static glm::vec3 ToDirectionVector(glm::quat quat)
    {
        return quat * glm::vec3(0.0f, 0.0f, -1.0f);
    }
    static glm::vec3 Mix(glm::vec3 start, glm::vec3 end, float t)
    {
        return glm::mix(start, end, t);
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
    auto& mathUtilClass = module.klass<MathUtil>("Math");
    mathUtilClass.funcStatic<&MathUtil::ToEuler>("ToEuler");
    mathUtilClass.funcStatic<&MathUtil::ToDirectionVector>("ToVector");
    mathUtilClass.funcStatic<&MathUtil::ToQuat>("ToQuat");
    mathUtilClass.funcStatic<&MathUtil::Mix>("Mix");
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

    entityClass.func<&WrenEntity::GetComponent<AudioEmitterComponent>>("GetAudioEmitterComponent");
    entityClass.func<&WrenEntity::AddComponent<AudioEmitterComponent>>("AddAudioEmitterComponent");

    entityClass.func<&WrenEntity::GetComponent<NameComponent>>("GetNameComponent");
    entityClass.func<&WrenEntity::AddComponent<NameComponent>>("AddNameComponent");

    entityClass.func<&WrenEntity::GetComponent<LifetimeComponent>>("GetLifetimeComponent");
    entityClass.func<&WrenEntity::AddComponent<LifetimeComponent>>("AddLifetimeComponent");

    entityClass.func<&WrenEntity::GetComponent<AnimationControlComponent>>("GetAnimationControlComponent");
}
