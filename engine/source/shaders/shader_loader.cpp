#include "shaders/shader_loader.hpp"

#include <fstream>

#include "vulkan_helper.hpp"

std::vector<std::byte> shader::ReadFile(std::string_view filename)
{
    // Open file at the end and interpret data as binary.
    std::ifstream file { filename.data(), std::ios::ate | std::ios::binary };

    // Failed to open file.
    if (!file.is_open())
        throw std::runtime_error("Failed opening shader file!");

    // Deduce file size based on read position (remember we opened the file at the end with the ate flag).
    size_t fileSize = file.tellg();

    // Allocate buffer with required file size.
    std::vector<std::byte> buffer(fileSize);

    // Place read position back to the start.
    file.seekg(0);

    // Read the buffer.
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    file.close();

    return buffer;
}

vk::ShaderModule shader::CreateShaderModule(const std::vector<std::byte>& byteCode, const vk::Device& device)
{
    vk::ShaderModuleCreateInfo createInfo {};
    createInfo.codeSize = byteCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(byteCode.data());

    vk::ShaderModule shaderModule {};
    util::VK_ASSERT(device.createShaderModule(&createInfo, nullptr, &shaderModule), "Failed creating shader module!");

    return shaderModule;
}
