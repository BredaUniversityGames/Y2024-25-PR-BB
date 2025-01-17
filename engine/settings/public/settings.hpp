#pragma once

#include <cereal/cereal.hpp>
#include <glm/vec3.hpp>

#include "serialization_helpers.hpp"

#define VERSION(x) constexpr static uint32_t V = x
#define CLASS_VERSION(x) CEREAL_CLASS_VERSION(x, x::V)

struct Settings
{
    VERSION(0);

    struct Fog
    {
        VERSION(0);

        glm::vec3 color { 0.5, 0.6, 0.7 };
        float density { 0.2f };
        float height { 0.3f };

        template <class Archive>
        void serialize(Archive& archive, const uint32_t version)
        {
            if (version != V)
            {
                return;
            }

            archive(color, density, height);
        }
    } fog;

    template <class Archive>
    void serialize(Archive& archive, const uint32_t version)
    {
        if (version != V)
        {
            return;
        }

        archive(fog);
    }
};

CLASS_VERSION(Settings);
CLASS_VERSION(Settings::Fog);

class SettingsStore
{
public:
    static SettingsStore& Instance();

    void Write();

    Settings settings;

private:
    SettingsStore();
};
