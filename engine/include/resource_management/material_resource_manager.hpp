#pragma once

#include "gpu_resources.hpp"
#include "resource_manager.hpp"

class VulkanBrain;

class MaterialResourceManager : public ResourceManager<Material>
{
public:
    explicit MaterialResourceManager(const VulkanBrain& brain);
    ResourceHandle<Material> Create(const MaterialCreation& creation);
    ResourceHandle<Material> Create(const Material& material) override { return ResourceManager<Material>::Create(material); }

private:
    const VulkanBrain& _brain;
};