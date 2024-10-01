#include "material_resource_manager.hpp"
#include "util.hpp"
#include "vulkan_helper.hpp"

material_resource_manager::material_resource_manager(const VulkanBrain& brain) :
    _brain(brain)
{}

ResourceHandle<Material> material_resource_manager::Create(const MaterialCreation& creation)
{
    Material materialResource{};
    materialResource.name = creation.name;

    materialResource.albedoMap = creation.albedoMap;
    materialResource.mrMap = creation.metallicRoughnessMap;
    materialResource.normalMap = creation.normalMap;
    materialResource.occlusionMap = creation.occlusionMap;
    materialResource.emissiveMap = creation.emissiveMap;

    Material::GPUInfo gpuInfo{};
    gpuInfo.useAlbedoMap = _brain.ImageResourceManager().IsValid(materialResource.albedoMap);
    gpuInfo.useMRMap = _brain.ImageResourceManager().IsValid(materialResource.mrMap);
    gpuInfo.useNormalMap = _brain.ImageResourceManager().IsValid(materialResource.normalMap);
    gpuInfo.useOcclusionMap = _brain.ImageResourceManager().IsValid(materialResource.occlusionMap);
    gpuInfo.useEmissiveMap = _brain.ImageResourceManager().IsValid(materialResource.emissiveMap);

    gpuInfo.albedoMapIndex = materialResource.albedoMap.index;
    gpuInfo.mrMapIndex = materialResource.albedoMap.index;
    gpuInfo.normalMapIndex = materialResource.albedoMap.index;
    gpuInfo.occlusionMapIndex = materialResource.albedoMap.index;
    gpuInfo.emissiveMapIndex = materialResource.albedoMap.index;

    gpuInfo.albedoFactor = creation.albedoFactor;
    gpuInfo.metallicFactor = creation.metallicFactor;
    gpuInfo.roughnessFactor = creation.roughnessFactor;
    gpuInfo.normalScale = creation.normalScale;
    gpuInfo.occlusionStrength = creation.occlusionStrength;
    gpuInfo.emissiveFactor = creation.emissiveFactor;

    CreateGPUBuffer(materialResource, gpuInfo);

    return ResourceManager::Create(materialResource);
}

void material_resource_manager::Destroy(ResourceHandle<Material> handle)
{
    if (IsValid(handle))
    {
        const Material* material = Access(handle);
        vmaDestroyBuffer(_brain.vmaAllocator, material->uniformBuffer, material->uniformAllocation);
        ResourceManager::Destroy(handle);
    }
}

void material_resource_manager::CreateGPUBuffer(Material& materialResource, const Material::GPUInfo& GPUMaterial)
{
    util::CreateBuffer(_brain, sizeof(Material::GPUInfo), vk::BufferUsageFlagBits::eUniformBuffer, materialResource.uniformBuffer, true, materialResource.uniformAllocation, VMA_MEMORY_USAGE_CPU_ONLY, materialResource.name);

    void* uniformPtr;
    util::VK_ASSERT(vmaMapMemory(_brain.vmaAllocator, materialResource.uniformAllocation, &uniformPtr), "Failed mapping memory for material UBO!");
    std::memcpy(uniformPtr, &GPUMaterial, sizeof(Material::GPUInfo));
    vmaUnmapMemory(_brain.vmaAllocator, materialResource.uniformAllocation);

    vk::DescriptorSetAllocateInfo allocateInfo {};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &materialResource.;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, &materialResource.descriptorSet),
        "Failed allocating material descriptor set!");

    vk::DescriptorBufferInfo uniformInfo {};
    uniformInfo.offset = 0;
    uniformInfo.buffer = materialResource.uniformBuffer;
    uniformInfo.range = sizeof(Material::GPUInfo);

    std::array<vk::WriteDescriptorSet, 1> writes;
    writes[0].dstSet = materialResource.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &uniformInfo;

    _brain.device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
}