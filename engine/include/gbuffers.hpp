#pragma once

#include "common.hpp"
#include "vulkan_helper.hpp"

class GBuffers
{
public:
    GBuffers(const VulkanBrain& brain, glm::uvec2 size);

    ~GBuffers();

    NON_MOVABLE(GBuffers);
    NON_COPYABLE(GBuffers);

    void Resize(glm::uvec2 size);

    const auto& Attachments() const
    {
        return _attachments;
    }

    ResourceHandle<Image> Depth() const
    {
        return _depthImage;
    }

    vk::Format DepthFormat() const
    {
        return _depthFormat;
    }

    ResourceHandle<Image> Shadow() const
    {
        return _shadowImage;
    }

    glm::uvec2 Size() const
    {
        return _size;
    }

    const vk::Rect2D& Scissor() const
    {
        return _scissor;
    }

    const vk::Viewport& Viewport() const
    {
        return _viewport;
    }

    void TransitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

private:
    const VulkanBrain& _brain;
    glm::uvec2 _size;

    std::array<ResourceHandle<Image>, DEFERRED_ATTACHMENT_COUNT> _attachments;

    ResourceHandle<Image> _depthImage;

    vk::Format _depthFormat;
    ResourceHandle<Image> _shadowImage;
    vk::Sampler _shadowSampler;

    vk::Format _shadowFormat;

    vk::Viewport _viewport;
    vk::Rect2D _scissor;

    void CreateGBuffers();

    void CreateDepthResources();

    void CreateShadowMapResources();

    void CreateViewportAndScissor();

    void CleanUp();
};