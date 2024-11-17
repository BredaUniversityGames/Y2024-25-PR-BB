#pragma once

#include "common.hpp"

#include <memory>

class BufferResourceManager;
class ImageResourceManager;
class MaterialResourceManager;
class SamplerResourceManager;
class MeshResourceManager;
class VulkanContext;

class GraphicsResources
{
public:
    GraphicsResources(const std::shared_ptr<VulkanContext>& vulkanContext);
    ~GraphicsResources();

    NON_COPYABLE(GraphicsResources);
    NON_MOVABLE(GraphicsResources);

    class MeshResourceManager& MeshResourceManager() { return *_meshResourceManager; }
    class MaterialResourceManager& MaterialResourceManager() { return *_materialResourceManager; }
    class ImageResourceManager& ImageResourceManager() { return *_imageResourceManager; }
    class BufferResourceManager& BufferResourceManager() { return *_bufferResourceManager; }
    class SamplerResourceManager& SamplerResourceManager() { return *_samplerResourceManager; }

    void Clean();

private:
    std::shared_ptr<VulkanContext> _vulkanContext;

    std::shared_ptr<class MeshResourceManager> _meshResourceManager;
    std::shared_ptr<class MaterialResourceManager> _materialResourceManager;
    std::shared_ptr<class ImageResourceManager> _imageResourceManager;
    std::shared_ptr<class BufferResourceManager> _bufferResourceManager;
    std::shared_ptr<class SamplerResourceManager> _samplerResourceManager;
};