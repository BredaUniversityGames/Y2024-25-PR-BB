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
    struct Draw
    {
        Draw(const std::shared_ptr<GraphicsContext>& context, const std::string& name, uint32_t instanceCount, vk::DescriptorSetLayout drawDSL, vk::DescriptorSetLayout visibilityDSL, vk::DescriptorSetLayout redirectDSL);

        ResourceHandle<Buffer> drawBuffer;
        ResourceHandle<Buffer> redirectBuffer;
        ResourceHandle<Buffer> visibilityBuffer;
        vk::DescriptorSet drawDescriptor;
        vk::DescriptorSet redirectDescriptor;
        vk::DescriptorSet visibilityDescriptor;

    private:
        vk::DescriptorSet CreateDescriptor(const std::shared_ptr<GraphicsContext>& context, vk::DescriptorSetLayout dsl, ResourceHandle<Buffer> buffer);
    };

    CameraBatch(const std::shared_ptr<GraphicsContext>& context, const std::string& name, const CameraResource& camera, ResourceHandle<GPUImage> depthImage, vk::DescriptorSetLayout drawDSL, vk::DescriptorSetLayout visibilityDSL, vk::DescriptorSetLayout redirectDSL);
    ~CameraBatch();

    const CameraResource& Camera() const { return _camera; }
    ResourceHandle<GPUImage> HZBImage() const { return _hzbImage; }
    ResourceHandle<GPUImage> DepthImage() const { return _depthImage; }
    Draw StaticDraw() const { return _staticDraw; }
    Draw SkinnedDraw() const { return _skinnedDraw; }

private:
    std::shared_ptr<GraphicsContext> _context;

    const CameraResource& _camera;

    ResourceHandle<GPUImage> _hzbImage;
    ResourceHandle<Sampler> _hzbSampler;

    ResourceHandle<GPUImage> _depthImage;

    Draw _staticDraw;
    Draw _skinnedDraw;
};
