#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

#include <memory>

class VulkanContext;

class MaterialResourceManager : public ResourceManager<Material>
{
public:
    explicit MaterialResourceManager(const std::shared_ptr<VulkanContext>& context);
    ResourceHandle<Material> Create(const MaterialCreation& creation);
    ResourceHandle<Material> Create(const Material& material) override { return ResourceManager<Material>::Create(material); }

private:
    std::shared_ptr<VulkanContext> _context;
};