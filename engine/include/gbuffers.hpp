#pragma once

#include <pch.hpp>

#include "vulkan_brain.hpp"
#include "vulkan_helper.hpp"

class GBuffers
{
public:
    GBuffers(const VulkanBrain& brain, glm::uvec2 size);

    ~GBuffers();

    NON_MOVABLE(GBuffers);
    NON_COPYABLE(GBuffers);

    void Resize(glm::uvec2 size);


    vk::Image GBuffersImageArray() const { return _brain.ImageResourceManager().Access(_gBuffersImage)->image; }
    const std::vector<vk::ImageView>& GBufferViews() const  { return _brain.ImageResourceManager().Access(_gBuffersImage)->views; }
    vk::ImageView GBufferView(uint32_t viewIndex) const { return _brain.ImageResourceManager().Access(_gBuffersImage)->views[viewIndex]; }
    vk::Image DepthImage() const { return _brain.ImageResourceManager().Access(_depthImage)->image; }
    vk::ImageView DepthImageView() const { return _brain.ImageResourceManager().Access(_depthImage)->views[0]; }


    ResourceHandle<Image> AlbedoM() const
    {
        return _albedoM;
    }

    ResourceHandle<Image> NormalR() const
    {
        return _normalR;
    }

    ResourceHandle<Image> EmissiveAO() const
    {
        return _emissiveAO;
    }

    ResourceHandle<Image> Position() const
    {
        return _position;
    }

    ResourceHandle<Image> Depth() const
    {
        return _depthImage;
    }

    vk::Format DepthFormat() const
    {
        return _depthFormat;
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

    static vk::Format GBufferFormat()
    {
        return vk::Format::eR16G16B16A16Sfloat;
    }

    void TransitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

private:
    const VulkanBrain& _brain;
    glm::uvec2 _size;


    ResourceHandle<Image> _gBuffersImage;

    ResourceHandle<Image> _albedoM;
    ResourceHandle<Image> _normalR;
    ResourceHandle<Image> _emissiveAO;
    ResourceHandle<Image> _position;

    ResourceHandle<Image> _depthImage;

    vk::Format _depthFormat;

    vk::Viewport _viewport;
    vk::Rect2D _scissor;

    static constexpr std::array<std::string_view, DEFERRED_ATTACHMENT_COUNT> _names = {
        "[VIEW] GBuffer RGB: Albedo A: Metallic", "[VIEW] GBuffer RGB: Normal A: Roughness",
        "[VIEW] GBuffer RGB: Emissive A: AO", "[VIEW] GBuffer RGB: Position A: Unused"
    };

    void CreateGBuffers();

    void CreateDepthResources();

    void CreateViewportAndScissor();

    void CleanUp();
};