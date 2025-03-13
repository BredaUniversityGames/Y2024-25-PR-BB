#pragma once

#include "wren_common.hpp"
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
}

}