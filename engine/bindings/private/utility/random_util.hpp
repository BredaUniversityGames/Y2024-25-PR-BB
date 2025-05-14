#pragma once

#include "perlin_noise.hpp"
#include "wren_common.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <random>

namespace bindings
{

class RandomUtil
{
public:
    static uint32_t Random()
    {
        return dist(rng);
    }

    static uint32_t RandomIndex(uint32_t start, uint32_t end)
    {
        uint32_t range = end - start;
        return Random() % range + start;
    }

    static float RandomFloat()
    {
        return static_cast<float>(Random()) / static_cast<float>(std::numeric_limits<uint32_t>::max());
    }

    static float RandomFloatRange(float min, float max)
    {
        std::uniform_real_distribution<float> range_dist(min, max);
        return range_dist(rng);
    }

    static glm::vec3 RandomVec3()
    {
        return { RandomFloat(), RandomFloat(), RandomFloat() };
    }

    static glm::vec3 RandomVec3Range(float min, float max)
    {
        return { RandomFloatRange(min, max), RandomFloatRange(min, max), RandomFloatRange(min, max) };
    }

    static glm::vec3 RandomVec3VectorRange(glm::vec3 min, glm::vec3 max)
    {
        return { RandomFloatRange(min.x, max.x), RandomFloatRange(min.y, max.y), RandomFloatRange(min.z, max.z) };
    }

    static glm::vec3 RandomPointOnUnitSphere()
    {
        // z is random in [-1, 1]
        float z = RandomFloatRange(-1.0f, 1.0f);

        // theta is random in [0, 2*pi]
        float theta = RandomFloatRange(0.0f, 2.0f * glm::pi<float>());

        // radius of the circle at height z
        float r = std::sqrt(1.0f - z * z);

        float x = r * std::cos(theta);
        float y = r * std::sin(theta);

        return glm::vec3(x, y, z);
    }

private:
    static std::random_device dev;
    static std::mt19937 rng;
    static std::uniform_int_distribution<std::mt19937::result_type> dist;
};

// Define static members outside the class
std::random_device RandomUtil::dev;
std::mt19937 RandomUtil::rng(RandomUtil::dev());
std::uniform_int_distribution<std::mt19937::result_type> RandomUtil::dist(0, std::numeric_limits<uint32_t>::max());

class Perlin
{
public:
    Perlin(uint32_t seed)
        : perlin(seed)
    {
    }

    siv::PerlinNoise perlin;

    float Noise1D(float x)
    {
        return perlin.noise1D_01(x);
    }
};

Perlin CreatePerlin(uint32_t seed) { return Perlin { seed }; }

inline void BindRandom(wren::ForeignModule& module)
{
    auto& randomUtilClass = module.klass<RandomUtil>("Random");
    randomUtilClass.funcStatic<&RandomUtil::Random>("Random");
    randomUtilClass.funcStatic<&RandomUtil::RandomFloat>("RandomFloat");
    randomUtilClass.funcStatic<&RandomUtil::RandomFloatRange>("RandomFloatRange");
    randomUtilClass.funcStatic<&RandomUtil::RandomVec3>("RandomVec3");
    randomUtilClass.funcStatic<&RandomUtil::RandomVec3Range>("RandomVec3Range");
    randomUtilClass.funcStatic<&RandomUtil::RandomVec3VectorRange>("RandomVec3VectorRange");
    randomUtilClass.funcStatic<&RandomUtil::RandomPointOnUnitSphere>("RandomPointOnUnitSphere");
    randomUtilClass.funcStatic<&RandomUtil::RandomIndex>("RandomIndex");

    auto& perlinClass = module.klass<Perlin>("Perlin");
    perlinClass.funcStaticExt<CreatePerlin>("new");
    perlinClass.func<&Perlin::Noise1D>("Noise1D");
}

}