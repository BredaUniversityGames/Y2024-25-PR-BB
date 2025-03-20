#pragma once
#include <string>
#include <vector>

#include "resource_manager.hpp"
#include "resources/sampler.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"

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

struct GPUImage
{
    GPUImage(const CPUImage& data, ResourceHandle<Sampler> textureSampler, const std::shared_ptr<VulkanContext>& context, SingleTimeCommands* const commands = nullptr);
    ~GPUImage();

    GPUImage(GPUImage&& other) noexcept;
    GPUImage& operator=(GPUImage&& other) noexcept;

    NON_COPYABLE(GPUImage);

    struct Layer
    {
        vk::ImageView view;
        std::vector<vk::ImageView> mipViews {};
    };

    vk::Image image {};
    std::vector<Layer> layerViews {};
    vk::ImageView view; // Same as first view in view, or refers to a cubemap view
    VmaAllocation allocation {};
    ResourceHandle<Sampler> sampler {};

    uint16_t width { 1 };
    uint16_t height { 1 };
    uint16_t depth { 1 };
    uint16_t layers { 1 };
    uint8_t mips { 1 };
    vk::ImageUsageFlags flags { 0 };
    bool isHDR;
    ImageType type;

    vk::Format format { vk::Format::eUndefined };
    vk::ImageType vkType { vk::ImageType::e2D };

    std::string name;

private:
    std::shared_ptr<VulkanContext> _context;
};