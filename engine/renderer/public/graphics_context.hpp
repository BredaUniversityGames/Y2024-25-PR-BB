#pragma once

#include "common.hpp"
#include "resource_manager.hpp"

#include <memory>
#include <vulkan_include.hpp>

#include "draw_stats.hpp"

class VulkanContext;
class GraphicsResources;
struct Buffer;
struct VulkanInitInfo;
struct Sampler;
struct GPUImage;

constexpr uint32_t MAX_BINDLESS_RESOURCES
    = 1024;

class GraphicsContext
{
public:
    GraphicsContext(const VulkanInitInfo& initInfo);
    ~GraphicsContext();

    NON_COPYABLE(GraphicsContext);
    NON_MOVABLE(GraphicsContext);

    std::shared_ptr<class VulkanContext> VulkanContext() const { return _vulkanContext; }
    std::shared_ptr<GraphicsResources> Resources() const { return _graphicsResources; }
    vk::DescriptorSetLayout BindlessLayout() const { return _bindlessLayout; }
    vk::DescriptorSet BindlessSet() const { return _bindlessSet; }

    DrawStats& GetDrawStats() { return _drawStats; }

    void UpdateBindlessSet();

private:
    std::shared_ptr<class VulkanContext> _vulkanContext;
    std::shared_ptr<GraphicsResources> _graphicsResources;

    // BINDLESS THINGS
    vk::DescriptorPool _bindlessPool;
    vk::DescriptorSetLayout _bindlessLayout;
    vk::DescriptorSet _bindlessSet;

    ResourceHandle<Sampler> _sampler;

    ResourceHandle<GPUImage> _fallbackImage;

    std::array<vk::DescriptorImageInfo, MAX_BINDLESS_RESOURCES> _bindlessImageInfos;
    std::array<vk::WriteDescriptorSet, MAX_BINDLESS_RESOURCES> _bindlessImageWrites;

    ResourceHandle<Buffer> _bindlessMaterialBuffer;
    vk::DescriptorBufferInfo _bindlessMaterialInfo;
    vk::WriteDescriptorSet _bindlessMaterialWrite;

    DrawStats _drawStats;

    void CreateBindlessDescriptorSet();
    void CreateBindlessMaterialBuffer();

    void UpdateBindlessImages();
    void UpdateBindlessMaterials();
};
