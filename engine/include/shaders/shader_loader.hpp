#pragma once
#include <vulkan/vulkan.hpp>
#include <string_view>
#include <vector>
#include <cstddef>

namespace shader
{
std::vector<std::byte> ReadFile(std::string_view filename);
vk::ShaderModule CreateShaderModule(const std::vector<std::byte>& byteCode, const vk::Device& device);
}