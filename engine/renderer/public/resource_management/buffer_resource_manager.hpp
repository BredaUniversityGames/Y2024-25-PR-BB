#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

#include <memory>

class VulkanContext;

class BufferResourceManager : public ResourceManager<Buffer>
{
public:
    explicit BufferResourceManager(const std::shared_ptr<VulkanContext>& context);
    ResourceHandle<Buffer> Create(const BufferCreation& creation);
    ResourceHandle<Buffer> Create(const Buffer& buffer) override { return ResourceManager<Buffer>::Create(buffer); }
    void Destroy(ResourceHandle<Buffer> handle) override;

private:
    std::shared_ptr<VulkanContext> _context;
};