#pragma once
#include <vector>
#include <string>

#include "vulkan_include.hpp"

enum class ImageType
{
    e2D,
    eDepth,
    eCubeMap,
    eShadowMap
};

struct CPUImage
{
    std::vector<std::byte> initialData;
    uint16_t width { 1 };
    uint16_t height { 1 };
    uint16_t depth { 1 };
    uint16_t layers { 1 };
    uint8_t mips { 1 };
    vk::ImageUsageFlags flags { 0 };
    bool isHDR;

    vk::Format format { vk::Format::eUndefined };
    ImageType type { ImageType::e2D };

    std::string name;

    /**
     * loads the pixel data, width, height and format from a specifed png file.
     * assigned format can be R8G8B8Unorm or vk::Format::eR8G8B8A8Unorm.
     * @param path filepath for the png file, .png extention required.
     */
    CPUImage& FromPNG(std::string_view path);
    CPUImage& SetData(std::vector<std::byte> data);
    CPUImage& SetSize(uint16_t width, uint16_t height, uint16_t depth = 1);
    CPUImage& SetMips(uint8_t mips);
    CPUImage& SetFlags(vk::ImageUsageFlags flags);
    CPUImage& SetFormat(vk::Format format);
    CPUImage& SetName(std::string_view name);
    CPUImage& SetType(ImageType type);
};