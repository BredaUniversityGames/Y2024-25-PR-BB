#pragma once

#include "resource_manager.hpp"
#include "gpu_resources.hpp"

class VulkanBrain;

class material_resource_manager : public ResourceManager<Material>
{
public:
    explicit material_resource_manager(const VulkanBrain& brain);
    ResourceHandle<Material> Create(const MaterialCreation& creation);
    ResourceHandle<Material> Create(const Material& material) override { return ResourceManager<Material>::Create(material); }

private:
    const VulkanBrain& _brain;
};