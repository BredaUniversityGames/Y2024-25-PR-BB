#pragma once

#include "resource_manager.hpp"
#include "vulkan_include.hpp"

class GPUScene;
class GraphicsContext;
struct GPUImage;
struct Buffer;
struct Sampler;
class CameraResource;

class CameraBatch
{
public:
    CameraBatch(const std::shared_ptr<GraphicsContext>& context, const std::string& name, const CameraResource& camera, ResourceHandle<GPUImage> depthImage, vk::DescriptorSetLayout drawDSL, vk::DescriptorSetLayout visibilityDSL, vk::DescriptorSetLayout redirectDSL);
    ~CameraBatch();

    const CameraResource& Camera() const { return _camera; }
    ResourceHandle<GPUImage> HZBImage() const { return _hzbImage; }
    ResourceHandle<GPUImage> DepthImage() const { return _depthImage; }
    ResourceHandle<Buffer> DrawBuffer() const { return _drawBuffer; }
    vk::DescriptorSet DrawBufferDescriptorSet() const { return _drawBufferDescriptorSet; }
    ResourceHandle<Buffer> OrderingBuffer(uint32_t index) const { return _orderingBuffers[index]; }
    ResourceHandle<Buffer> VisibilityBuffer() const { return _visibilityBuffer; }
    vk::DescriptorSet VisibilityBufferDescriptorSet() const { return _visibilityDescriptorSet; }
    ResourceHandle<Buffer> RedirectBuffer() const { return _redirectBuffer; }
    vk::DescriptorSet RedirectBufferDescriptorSet() const { return _redirectBufferDescriptorSet; }

private:
    std::shared_ptr<GraphicsContext> _context;

    const CameraResource& _camera;

    ResourceHandle<GPUImage> _hzbImage;
    ResourceHandle<Sampler> _hzbSampler;

    ResourceHandle<GPUImage> _depthImage;

    ResourceHandle<Buffer> _drawBuffer;
    ResourceHandle<Buffer> _redirectBuffer;
    vk::DescriptorSet _drawBufferDescriptorSet;
    vk::DescriptorSet _redirectBufferDescriptorSet;

    std::array<ResourceHandle<Buffer>, 2> _orderingBuffers;

    ResourceHandle<Buffer> _visibilityBuffer;
    vk::DescriptorSet _visibilityDescriptorSet;

    void CreateDrawBufferDescriptorSet(vk::DescriptorSetLayout drawDSL);
};
