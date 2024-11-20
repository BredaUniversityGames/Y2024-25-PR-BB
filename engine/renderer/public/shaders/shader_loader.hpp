#pragma once
#include "vulkan/vulkan.hpp"
#include <vector>

namespace shader
{
std::vector<std::byte> ReadFile(std::string_view filename);
vk::ShaderModule CreateShaderModule(const std::vector<std::byte>& byteCode, const vk::Device& device);
}