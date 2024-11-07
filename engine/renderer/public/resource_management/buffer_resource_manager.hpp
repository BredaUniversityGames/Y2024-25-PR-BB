#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

class VulkanContext;

class BufferResourceManager : public ResourceManager<Buffer>
{
public:
    explicit BufferResourceManager(const VulkanContext& brain);
    ResourceHandle<Buffer> Create(const BufferCreation& creation);
    ResourceHandle<Buffer> Create(const Buffer& buffer) override { return ResourceManager<Buffer>::Create(buffer); }
    void Destroy(ResourceHandle<Buffer> handle) override;

private:
    const VulkanContext& _brain;
};