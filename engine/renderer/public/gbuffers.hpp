#pragma once

#include "common.hpp"
#include "constants.hpp"
#include "resource_manager.hpp"

#include <array>
#include <glm/vec2.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>

struct GPUImage;
class GraphicsContext;
struct Sampler;

class GBuffers
{
public:
    GBuffers(const std::shared_ptr<GraphicsContext>& context, glm::uvec2 size);
    ~GBuffers();

    NON_MOVABLE(GBuffers);
    NON_COPYABLE(GBuffers);

    void Resize(glm::uvec2 size);

    const auto& Attachments() const { return _attachments; }
    ResourceHandle<GPUImage> Depth() const { return _depthImage; }
    vk::Format DepthFormat() const { return _depthFormat; }
    glm::uvec2 Size() const { return _size; }
    const vk::Rect2D& Scissor() const { return _scissor; }
    const vk::Viewport& Viewport() const { return _viewport; }

    void TransitionLayout(vk::CommandBuffer commandBuffer, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

private:
    std::shared_ptr<GraphicsContext> _context;
    glm::uvec2 _size;

    std::array<ResourceHandle<GPUImage>, DEFERRED_ATTACHMENT_COUNT> _attachments;

    ResourceHandle<GPUImage> _depthImage;

    vk::Format _depthFormat;

    vk::Viewport _viewport;
    vk::Rect2D _scissor;

    void CreateGBuffers();
    void CreateDepthResources();
    void CreateViewportAndScissor();
    void CleanUp();
};