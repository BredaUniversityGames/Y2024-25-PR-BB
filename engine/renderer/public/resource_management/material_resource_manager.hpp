#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

class VulkanContext;

class MaterialResourceManager : public ResourceManager<Material>
{
public:
    explicit MaterialResourceManager(const VulkanContext& brain);
    ResourceHandle<Material> Create(const MaterialCreation& creation);
    ResourceHandle<Material> Create(const Material& material) override { return ResourceManager<Material>::Create(material); }

private:
    const VulkanContext& _brain;
};