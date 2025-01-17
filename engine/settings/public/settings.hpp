#pragma once

#include <cereal/cereal.hpp>
#include <glm/vec3.hpp>
#include <visit_struct/visit_struct.hpp>

#include "log.hpp"
#include "serialization_helpers.hpp"

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

struct Settings
{
    VERSION(0);

    struct Fog
    {
        VERSION(0);

        glm::vec3 color { 0.5, 0.6, 0.7 };
        float density { 0.2f };
        float height { 0.3f };
    } fog;
};

VISITABLE_STRUCT(Settings::Fog, color, density, height);
CLASS_SERIALIZE_VERSION(Settings::Fog);
CLASS_VERSION(Settings::Fog);

VISITABLE_STRUCT(Settings, fog);
CLASS_SERIALIZE_VERSION(Settings);
CLASS_VERSION(Settings);

class SettingsStore
{
public:
    static SettingsStore& Instance();

    void Write();

    Settings settings;

private:
    SettingsStore();
};
