#include "shader_reflector.hpp"

ShaderReflector::ShaderReflector(const std::vector<std::byte>& spvBytes)
{
    SpvReflectResult result = spvReflectCreateShaderModule(spvBytes.size(), spvBytes.data(), &_module);
    assert(result == SpvReflectResult::SPV_REFLECT_RESULT_SUCCESS);
}

ShaderReflector::~ShaderReflector()
{
    spvReflectDestroyShaderModule(&_module);
}

void ShaderReflector::Reflect()
{
}
