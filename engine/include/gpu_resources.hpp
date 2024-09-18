#pragma once
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

struct ImageCreation
{

    std::byte* initialData{ nullptr };
    uint16_t width{ 1 };
    uint16_t height{ 1 };
    uint16_t depth{ 1 };
    uint16_t layers{ 1 };
    uint8_t mips{ 1 };
    vk::ImageUsageFlags flags{ 0 };
    bool isHDR;

    vk::Format format{ vk::Format::eUndefined };
    vk::ImageType type{ vk::ImageType::e2D };

    std::string name;

    ImageCreation& LoadFromFile(std::string_view file_path);
    
    ImageCreation& SetData(std::byte* data);
    ImageCreation& SetSize(uint16_t width, uint16_t height, uint16_t depth = 1);
    ImageCreation& SetMips(uint8_t mips);
    ImageCreation& SetFlags(vk::ImageUsageFlags flags);
    ImageCreation& SetFormat(vk::Format format);
    ImageCreation& SetName(std::string_view name);
};

struct Image
{
    vk::Image image{};
    std::vector<vk::ImageView> views{};
    VmaAllocation allocation{};

    uint16_t width{ 1 };
    uint16_t height{ 1 };
    uint16_t depth{ 1 };
    uint16_t layers{ 1 };
    uint8_t mips{ 1 };
    vk::ImageUsageFlags flags{ 0 };
    bool isHDR;

    vk::Format format{ vk::Format::eUndefined };
    vk::ImageType type{vk::ImageType::e2D };

    std::string name;
};


