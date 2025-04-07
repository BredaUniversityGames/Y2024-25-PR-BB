#pragma once

#include "wren_common.hpp"
#include <glm/glm.hpp>
#include <random>

namespace bindings
{

class RandomUtil
{
public:
    static uint32_t Random()
    {
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> dist6(0, std::numeric_limits<uint32_t>::max());

        return dist6(rng);
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
        float v = RandomFloat();
        return v * (max - min) + min;
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
};

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
}

}