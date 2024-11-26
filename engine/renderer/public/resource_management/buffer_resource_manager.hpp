#pragma once

#include "common.hpp"
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

private:
    std::shared_ptr<VulkanContext> _context;
};