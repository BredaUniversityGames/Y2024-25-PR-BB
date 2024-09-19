#pragma once
#include "vk_mem_alloc.h"
#include "vulkan/vulkan.hpp"

enum class ImageType
{
    e2D, e2DArray, eCubeMap
};

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
    ImageType type{ ImageType::e2D };
    vk::Sampler sampler{ nullptr };

    std::string name;

    ImageCreation& SetData(std::byte* data);
    ImageCreation& SetSize(uint16_t width, uint16_t height, uint16_t depth = 1);
    ImageCreation& SetMips(uint8_t mips);
    ImageCreation& SetFlags(vk::ImageUsageFlags flags);
    ImageCreation& SetFormat(vk::Format format);
    ImageCreation& SetName(std::string_view name);
    ImageCreation& SetType(ImageType type);
    ImageCreation& SetSampler(vk::Sampler sampler);
};

struct Image
{
    vk::Image image{};
    std::vector<vk::ImageView> views{};
    vk::ImageView view; // Same as first view in view, or refers to a cubemap view
    VmaAllocation allocation{};
    vk::Sampler sampler{ nullptr };

    uint16_t width{ 1 };
    uint16_t height{ 1 };
    uint16_t depth{ 1 };
    uint16_t layers{ 1 };
    uint8_t mips{ 1 };
    vk::ImageUsageFlags flags{ 0 };
    bool isHDR;
    ImageType type;

    vk::Format format{ vk::Format::eUndefined };
    vk::ImageType vkType{vk::ImageType::e2D };

    std::string name;
};


