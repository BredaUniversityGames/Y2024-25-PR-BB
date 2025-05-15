#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/list.hpp>
#include <cereal/types/vector.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <visit_struct/visit_struct.hpp>

#define CLASS_SERIALIZE(Type)                                                 \
    template <class Archive>                                                  \
    void serialize(Archive& archive, Type& obj)                               \
    {                                                                         \
        visit_struct::for_each(obj, [&archive](const char* name, auto& value) \
            { archive(cereal::make_nvp(name, value)); });                     \
    }

VISITABLE_STRUCT(glm::vec2, x, y);
VISITABLE_STRUCT(glm::vec3, x, y, z);
VISITABLE_STRUCT(glm::vec4, x, y, z, w);
VISITABLE_STRUCT(glm::ivec2, x, y);
VISITABLE_STRUCT(glm::ivec3, x, y, z);
VISITABLE_STRUCT(glm::ivec4, x, y, z, w);
VISITABLE_STRUCT(glm::uvec2, x, y);
VISITABLE_STRUCT(glm::uvec3, x, y, z);
VISITABLE_STRUCT(glm::uvec4, x, y, z, w);
VISITABLE_STRUCT(glm::dvec2, x, y);
VISITABLE_STRUCT(glm::dvec3, x, y, z);
VISITABLE_STRUCT(glm::dvec4, x, y, z, w);
VISITABLE_STRUCT(glm::quat, w, x, y, z);
VISITABLE_STRUCT(glm::dquat, w, x, y, z);

namespace glm
{
CLASS_SERIALIZE(glm::vec2);
CLASS_SERIALIZE(glm::vec3);
CLASS_SERIALIZE(glm::vec4);
CLASS_SERIALIZE(glm::ivec2);
CLASS_SERIALIZE(glm::ivec3);
CLASS_SERIALIZE(glm::ivec4);
CLASS_SERIALIZE(glm::uvec2);
CLASS_SERIALIZE(glm::uvec3);
CLASS_SERIALIZE(glm::uvec4);
CLASS_SERIALIZE(glm::dvec2);
CLASS_SERIALIZE(glm::dvec3);
CLASS_SERIALIZE(glm::dvec4);
CLASS_SERIALIZE(glm::quat);
CLASS_SERIALIZE(glm::dquat);
}

template <typename Archive, typename T, size_t S>
void serialize(Archive& archive, std::array<T, S>& m)
{
    cereal::size_type s = S;
    archive(cereal::make_size_tag(s));
    if (s != S)
    {
        throw std::runtime_error("array has incorrect length");
    }
    for (auto& i : m)
    {
        archive(i);
    }
}

#define VERSION(x) constexpr static uint32_t V = x
#define CLASS_VERSION(x) CEREAL_CLASS_VERSION(x, x::V)

#define CLASS_SERIALIZE_VERSION(Type)                                                   \
    template <class Archive>                                                            \
    void serialize(Archive& archive, Type& obj, const uint32_t version)                 \
    {                                                                                   \
        if (version != Type::V)                                                         \
        {                                                                               \
            bblog::warn("Outdated serialization for: {}", visit_struct::get_name(obj)); \
            return;                                                                     \
        }                                                                               \
                                                                                        \
        visit_struct::for_each(obj, [&archive](const char* name, auto& value)           \
            { archive(cereal::make_nvp(name, value)); });                               \
    }