#pragma once

#include "common.hpp"

class ShaderReflector
{
public:
    ShaderReflector(const std::vector<std::byte>& spvBytes);
    ~ShaderReflector();

    void Reflect();

    NON_COPYABLE(ShaderReflector);
    NON_MOVABLE(ShaderReflector);

private:
    SpvReflectShaderModule _module;
};