#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

#include <memory>

class VulkanContext;

class BufferResourceManager final : public ResourceManager<Buffer>
{
public:
    explicit BufferResourceManager(const std::shared_ptr<VulkanContext>& context);
    ~BufferResourceManager() final = default;

    ResourceHandle<Buffer> Create(const BufferCreation& creation);
    ResourceHandle<Buffer> Create(const Buffer& buffer) final { return ResourceManager<Buffer>::Create(buffer); }
    void Destroy(ResourceHandle<Buffer> handle) final;

private:
    std::shared_ptr<VulkanContext> _context;
};