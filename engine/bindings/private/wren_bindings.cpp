#include "wren_bindings.hpp"

#include "utility/wren_entity.hpp"
#include "wren_engine.hpp"

#include "ecs_module.hpp"
#include "time_module.hpp"

#include "components/name_component.hpp"
#include "components/transform_component.hpp"
#include "components/transform_helpers.hpp"

namespace bindings
{
void BindMath(wren::ForeignModule& module);
void BindEntity(wren::ForeignModule& module);

float TimeModuleGetDeltatime(TimeModule& self)
{
    return self.GetDeltatime().count();
}

WrenEntity CreateEntity(ECSModule& self)
{
    return { self.GetRegistry().create(), &self.GetRegistry() };
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

void TransformComponentSetTranslation(WrenComponent<TransformComponent>& component, const glm::vec3& translation)
{
    TransformHelpers::SetLocalPosition(*component.entity.registry, component.entity.entity, translation);
}

std::string NameComponentGetName(WrenComponent<NameComponent>& nameComponent)
{
    return nameComponent.component->name;
}

}

void BindEngineAPI(wren::ForeignModule& module)
{

    bindings::BindMath(module);
    bindings::BindEntity(module);

    // Add modules here to expose them in scripting
    {
        auto& engineAPI = module.klass<WrenEngine>("Engine");
        engineAPI.func<&WrenEngine::GetModule<TimeModule>>("GetTime");
        engineAPI.func<&WrenEngine::GetModule<ECSModule>>("GetECS");
    }

    // Time Module
    {
        auto& time = module.klass<TimeModule>("TimeModule");
        time.funcExt<bindings::TimeModuleGetDeltatime>("GetDeltatime");
    }

    // ECS module
    {
        // ECS class
        auto& wren_class = module.klass<ECSModule>("ECS");
        wren_class.funcExt<bindings::CreateEntity>("NewEntity");
        wren_class.funcExt<bindings::GetEntityByName>("GetEntityByName");

        // Name class
        auto& nameClass = module.klass<WrenComponent<NameComponent>>("NameComponent");
        nameClass.propReadonlyExt<bindings::NameComponentGetName>("name");

        // Transform component
        auto& transformClass = module.klass<WrenComponent<TransformComponent>>("TransformComponent");
        transformClass.funcExt<bindings::TransformComponentSetTranslation>("SetTranslation");
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

void bindings::BindEntity(wren::ForeignModule& module)
{
    // Entity class
    auto& entityClass = module.klass<WrenEntity>("Entity");

    entityClass.func<&WrenEntity::GetComponent<TransformComponent>>("GetTransformComponent");
    entityClass.func<&WrenEntity::AddComponent<TransformComponent>>("AddTransformComponent");

    entityClass.func<&WrenEntity::GetComponent<NameComponent>>("GetNameComponent");
    entityClass.func<&WrenEntity::AddComponent<NameComponent>>("AddNameComponent");
}
