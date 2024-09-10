#pragma once

#include <vector>
#include <string_view>
#include <vulkan/vulkan.hpp>

namespace shader
{
    std::vector<std::byte> ReadFile(std::string_view filename);
    vk::ShaderModule CreateShaderModule(const std::vector<std::byte>& byteCode, const vk::Device& device);
}