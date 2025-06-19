#include "shaders/shader_loader.hpp"

#include "vulkan_helper.hpp"

#include <file_io.hpp>
#include <fstream>

std::vector<std::byte> shader::ReadFile(std::string_view filename)
{
    // Open file at the end and interpret data as binary.
    auto stream = fileIO::OpenReadStream(std::string { filename });

    // Failed to open file.
    if (!stream.has_value())
        throw std::runtime_error("Failed opening shader file!");

    // Deduce file size based on read position (remember we opened the file at the end with the ate flag).
    stream.value().seekg(0, std::ios::end);
    size_t fileSize = stream.value().tellg();

    // Allocate buffer with required file size.
    std::vector<std::byte> buffer(fileSize);

    // Place read position back to the start.
    stream.value().seekg(0);

    // Read the buffer.
    stream.value().read(reinterpret_cast<char*>(buffer.data()), fileSize);

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
