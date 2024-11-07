#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

class VulkanBrain;

class BufferResourceManager : public ResourceManager<Buffer>
{
public:
    explicit BufferResourceManager(const VulkanBrain& brain);
    ResourceHandle<Buffer> Create(const BufferCreation& creation);
    ResourceHandle<Buffer> Create(const Buffer& buffer) override { return ResourceManager<Buffer>::Create(buffer); }
    void Destroy(ResourceHandle<Buffer> handle) override;

private:
    const VulkanBrain& _brain;
};