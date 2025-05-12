#pragma once
#include "log.hpp"
#include "serialization_helpers.hpp"

#define CLASS_SERIALIZE_VERSION(Type, Version)                                          \
    CEREAL_CLASS_VERSION(Type, Version)                                                 \
    template <class Archive>                                                            \
    void serialize(Archive& archive, Type& obj, const uint32_t version)                 \
    {                                                                                   \
        if (version != Version)                                                         \
        {                                                                               \
            bblog::warn("Outdated serialization for: {}", visit_struct::get_name(obj)); \
            return;                                                                     \
        }                                                                               \
                                                                                        \
        visit_struct::for_each(obj, [&archive](const char* name, auto& value)           \
            { archive(cereal::make_nvp(name, value)); });                               \
    }

struct GameSettings
{
    bool framerateCounter = true;
};

VISITABLE_STRUCT(GameSettings);
CLASS_SERIALIZE_VERSION(GameSettings, 1);