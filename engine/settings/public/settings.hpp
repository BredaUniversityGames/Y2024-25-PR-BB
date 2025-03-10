#pragma once

#include <cereal/cereal.hpp>
#include <visit_struct/visit_struct.hpp>

#include "log.hpp"
#include "serialization_helpers.hpp"
#include "tonemapping_functions.hpp"

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

    struct SSAO
    {
        VERSION(0);

        float strength { 2.0f };
        float bias { 0.01f };
        float radius { 0.2f };
        float minDistance { 1.0f };
        float maxDistance { 3.0f };
    } ssao;

    struct FXAA
    {
        VERSION(0);

        bool enableFXAA = true;
        float edgeThresholdMin = 0.0312;
        float edgeThresholdMax = 0.125;
        float subPixelQuality = 1.2f;
        int32_t iterations = 12;
    } fxaa;

    struct Bloom
    {
        VERSION(0);

        glm::vec3 colorWeights { 0.2126f, 0.7152f, 0.0722f };
        float strength { 0.8f };
        float gradientStrength { 0.2f };
        float maxBrightnessExtraction { 5.0f };
    } bloom;

    struct Tonemapping
    {
        VERSION(6);

        TonemappingFunctions tonemappingFunction { TonemappingFunctions::eAces };
        float exposure { 1.0f };

        bool enableVignette;
        float vignetteIntensity { 0.2f };

        bool enableLensDistortion;
        float lensDistortionIntensity;
        float lensDistortionCubicIntensity;
        float screenScale;

        bool enableToneAdjustments;
        float brightness;
        float contrast;
        float saturation;
        float vibrance;
        float hue;

        // pixelization
        bool enablePixelization;
        float minPixelSize;
        float maxPixelSize;
        float pixelizationLevels;
        float pixelizationDepthBias;

        // fixed palette
        bool enablePalette;
        float ditherAmount = 0.15f;
        float paletteAmount = 0.8f;
        glm::vec4 palette[5] = {
            glm::vec4(14.0f, 193.0f, 4.0f, 256.0f) / 256.0f, // Black
            glm::vec4(6.0f, 6.0f, 6.0f, 256.0f) / 256.0f, // White
            glm::vec4(94.0f, 43.0f, 22.0f, 256.0f) / 256.0f, // Red
            glm::vec4(172.0f, 18.0f, 18.0f, 256.0f) / 256.0f,
            glm::vec4(128.0f, 128.0f, 128.0f, 256.0f) / 256.0f
        };

        glm::vec4 skyColor { 0.0f, 0.0f, 0.0f, 1.0f };
        glm::vec4 sunColor { 0.0f, 0.0f, 0.0f, 1.0f };
        glm::vec4 cloudsColor { 0.0f, 0.0f, 0.0f, 1.0f };
        glm::vec4 voidColor { 0.0f, 0.0f, 0.0f, 1.0f };

    } tonemapping;
};

VISITABLE_STRUCT(Settings::Fog, color, density, height);
CLASS_SERIALIZE_VERSION(Settings::Fog);
CLASS_VERSION(Settings::Fog);

VISITABLE_STRUCT(Settings::SSAO, strength, bias, radius, minDistance, maxDistance);
CLASS_SERIALIZE_VERSION(Settings::SSAO);
CLASS_VERSION(Settings::SSAO)

VISITABLE_STRUCT(Settings::FXAA, enableFXAA, edgeThresholdMin, edgeThresholdMax, subPixelQuality, iterations);
CLASS_SERIALIZE_VERSION(Settings::FXAA);
CLASS_VERSION(Settings::FXAA);

VISITABLE_STRUCT(Settings::Bloom, colorWeights, strength, gradientStrength, maxBrightnessExtraction);
CLASS_SERIALIZE_VERSION(Settings::Bloom);
CLASS_VERSION(Settings::Bloom);

VISITABLE_STRUCT(Settings::Tonemapping, tonemappingFunction, exposure, enableVignette, vignetteIntensity, enableLensDistortion, lensDistortionIntensity, lensDistortionCubicIntensity, screenScale, enableToneAdjustments, brightness, contrast, saturation, vibrance, hue, enablePixelization, minPixelSize, maxPixelSize, pixelizationLevels, pixelizationDepthBias, enablePalette, ditherAmount, paletteAmount, palette, skyColor, sunColor, cloudsColor, voidColor);
CLASS_SERIALIZE_VERSION(Settings::Tonemapping);
CLASS_VERSION(Settings::Tonemapping);

VISITABLE_STRUCT(Settings, fog, ssao, fxaa, bloom, tonemapping);
CLASS_SERIALIZE_VERSION(Settings);
CLASS_VERSION(Settings);
