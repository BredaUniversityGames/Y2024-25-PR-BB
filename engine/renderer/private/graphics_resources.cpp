#include "graphics_resources.hpp"

#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/mesh_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"

GraphicsResources::GraphicsResources(const std::shared_ptr<VulkanContext>& vulkanContext)
    : _vulkanContext(vulkanContext)
{
    _samplerResourceManager = std::make_shared<class SamplerResourceManager>(_vulkanContext);
    _bufferResourceManager = std::make_shared<class BufferResourceManager>(_vulkanContext);
    _imageResourceManager = std::make_shared<class ImageResourceManager>(_vulkanContext);
    _materialResourceManager = std::make_shared<class MaterialResourceManager>(_imageResourceManager);
    _meshResourceManager = std::make_shared<class MeshResourceManager>();
}

void GraphicsResources::Clean()
{
    _meshResourceManager->Clean();
    _materialResourceManager->Clean();
    _imageResourceManager->Clean();
    _bufferResourceManager->Clean();
    _samplerResourceManager->Clean();
}

GraphicsResources::~GraphicsResources()
{
    _meshResourceManager.reset();
    _materialResourceManager.reset();
    _imageResourceManager.reset();
    _bufferResourceManager.reset();
    _samplerResourceManager.reset();
}
