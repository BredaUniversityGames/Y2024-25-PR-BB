#pragma once

#include "common.hpp"

#include <memory>

class Mesh;
class BufferResourceManager;
class ImageResourceManager;
class MaterialResourceManager;
template <typename T>
class ResourceManager;
class VulkanContext;

class GraphicsResources
{
public:
    GraphicsResources(const std::shared_ptr<VulkanContext>& vulkanContext);
    ~GraphicsResources();

    NON_COPYABLE(GraphicsResources);
    NON_MOVABLE(GraphicsResources);

    BufferResourceManager& BufferResourceManager() { return *_bufferResourceManager; }
    ImageResourceManager& ImageResourceManager() { return *_imageResourceManager; }
    MaterialResourceManager& MaterialResourceManager() { return *_materialResourceManager; }
    ResourceManager<Mesh>& MeshResourceManager() { return *_meshResourceManager; }

private:
    std::shared_ptr<VulkanContext> _vulkanContext;

    std::shared_ptr<class BufferResourceManager> _bufferResourceManager;
    std::shared_ptr<class ImageResourceManager> _imageResourceManager;
    std::shared_ptr<class MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<ResourceManager<Mesh>> _meshResourceManager;
};