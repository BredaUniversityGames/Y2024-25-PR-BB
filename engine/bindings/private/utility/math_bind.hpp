#pragma once
#include "wren_common.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace bindings
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
T Mul(T& lhs, const T& rhs) { return lhs * rhs; }

template <typename T, typename U>
T LeftRetDiv(T& lhs, const U& rhs) { return lhs / rhs; }

template <typename T, typename U>
U RightRetDiv(T& lhs, const U& rhs) { return lhs / rhs; }

template <typename T, typename U>
T LeftRetMul(T& lhs, const U& rhs) { return lhs * rhs; }

template <typename T, typename U>
U RightRetMul(T& lhs, const U& rhs) { return lhs * rhs; }

template <typename T>
bool Equals(T& lhs, const T& rhs) { return lhs == rhs; }

template <typename T>
bool NotEquals(T& lhs, const T& rhs) { return lhs != rhs; }

template <typename T>
T Normalized(T& v) { return glm::normalize(v); }

template <typename T>
float Length(T& v) { return glm::length(v); }

template <typename T>
float Distance(T& v) { return glm::distance(v); }

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
    static float MixFloat(float start, float end, float t)
    {
        return glm::mix(start, end, t);
    }
    static float Dot(glm::vec3 a, glm::vec3 b)
    {
        return glm::dot(a, b);
    }
    static glm::vec3 Cross(glm::vec3 a, glm::vec3 b)
    {
        return glm::cross(a, b);
    }
    static float Clamp(float a, float min, float max)
    {
        return glm::clamp(a, min, max);
    }
    static float Sqrt(float a)
    {
        return glm::sqrt(a);
    }
    static float Abs(float a)
    {
        return glm::abs(a);
    }
    static float Max(const float a, const float b)
    {
        return glm::max(a, b);
    }
    static float Min(const float a, const float b)
    {
        return glm::min(a, b);
    }
    static float Sin(const float a)
    {
        return glm::sin(a);
    }
    static float Cos(const float a)
    {
        return glm::cos(a);
    }
    static float Radians(const float a)
    {
        return glm::radians(a);
    }
    static glm::quat AngleAxis(const float a, glm::vec3 vec)
    {
        return glm::angleAxis(a, vec);
    }
    static glm::vec3 RotateForwardVector(glm::vec3 forward, glm::vec2 rotation, glm::vec3 up)
    {
        glm::vec3 normalizedForward = glm::normalize(forward);
        glm::vec3 normalizedUp = glm::normalize(up);

        glm::quat yawQuat = glm::angleAxis(glm::radians(rotation.x), normalizedUp);
        glm::vec3 rotatedForward = yawQuat * normalizedForward;

        glm::vec3 right = glm::normalize(glm::cross(normalizedUp, rotatedForward));

        glm::quat pitchQuat = glm::angleAxis(glm::radians(rotation.y), right);
        rotatedForward = pitchQuat * rotatedForward;

        return glm::normalize(rotatedForward);
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

    static float Distance(glm::vec3 pos1, glm::vec3 pos2)
    {
        return glm::distance(pos1, pos2);
    }
};

template <typename T>
void BindVectorTypeOperations(wren::ForeignKlassImpl<T>& klass)
{
    klass.template funcStaticExt<Default<T>>("Default");
    klass.template funcExt<Add<T>>(wren::OPERATOR_ADD);
    klass.template funcExt<Sub<T>>(wren::OPERATOR_SUB);
    klass.template funcExt<Neg<T>>(wren::OPERATOR_NEG);
    klass.template funcExt<Mul<T>>(wren::OPERATOR_MUL);
    klass.template funcExt<Equals<T>>(wren::OPERATOR_EQUAL);
    klass.template funcExt<NotEquals<T>>(wren::OPERATOR_NOT_EQUAL);
    klass.template funcExt<Normalized<T>>("normalize");
    klass.template funcExt<Length<T>>("length");
    klass.template funcExt<LeftRetMul<T, float>>("mulScalar");
    klass.template funcExt<LeftRetDiv<T, float>>("divScalar");
}

inline void BindMath(wren::ForeignModule& module)
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

        vector3.funcExt<LeftRetMul<glm::vec3, glm::quat>>("mulQuat");
        vector3.funcExt<RightRetMul<glm::vec3, glm::quat>>("mulQuatRetQuat");
    }

    {
        auto& quat = module.klass<glm::quat>("Quat");
        quat.ctor<float, float, float, float>();
        quat.var<&glm::quat::w>("w");
        quat.var<&glm::quat::x>("x");
        quat.var<&glm::quat::y>("y");
        quat.var<&glm::quat::z>("z");
        BindVectorTypeOperations(quat);

        quat.funcExt<RightRetMul<glm::quat, glm::vec3>>("mulVec3");
        quat.funcExt<LeftRetMul<glm::quat, glm::vec3>>("mulVec3RetQuat");
    }

    {
        auto& mathUtilClass = module.klass<MathUtil>("Math");
        mathUtilClass.funcStatic<&MathUtil::ToEuler>("ToEuler");
        mathUtilClass.funcStatic<&MathUtil::ToDirectionVector>("ToVector");
        mathUtilClass.funcStatic<&MathUtil::ToQuat>("ToQuat");
        mathUtilClass.funcStatic<&MathUtil::Mix>("Mix");
        mathUtilClass.funcStatic<&MathUtil::MixFloat>("MixFloat");
        mathUtilClass.funcStatic<&MathUtil::Dot>("Dot");
        mathUtilClass.funcStatic<&MathUtil::Cross>("Cross");
        mathUtilClass.funcStatic<&MathUtil::Clamp>("Clamp");
        mathUtilClass.funcStatic<&MathUtil::Max>("Max");
        mathUtilClass.funcStatic<&MathUtil::Min>("Min");
        mathUtilClass.funcStatic<&MathUtil::Sin>("Sin");
        mathUtilClass.funcStatic<&MathUtil::Cos>("Cos");
        mathUtilClass.funcStatic<&MathUtil::Radians>("Radians");
        mathUtilClass.funcStatic<&MathUtil::AngleAxis>("AngleAxis");
        mathUtilClass.funcStatic<&MathUtil::RotateForwardVector>("RotateForwardVector");
        mathUtilClass.funcStatic<&MathUtil::Sqrt>("Sqrt");
        mathUtilClass.funcStatic<&MathUtil::Abs>("Abs");
        mathUtilClass.funcStatic<&MathUtil::PI>("PI");
        mathUtilClass.funcStatic<&MathUtil::TwoPI>("TwoPI");
        mathUtilClass.funcStatic<&MathUtil::HalfPI>("HalfPI");
        mathUtilClass.funcStatic<&MathUtil::Distance>("Distance");
    }
}

};