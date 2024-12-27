#include "graphics_context.hpp"

#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/material_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

GraphicsContext::GraphicsContext(const VulkanInitInfo& initInfo)
    : _drawStats()
{
    _vulkanContext = std::make_shared<class VulkanContext>(initInfo);
    _graphicsResources = std::make_shared<GraphicsResources>(_vulkanContext);

    CreateBindlessMaterialBuffer();
    CreateBindlessDescriptorSet();

    const uint32_t size { 2 };
    std::vector<std::byte> data;
    data.assign(size * size * 4, std::byte {});
    CPUImage imageData {};
    imageData
        .SetSize(size, size)
        .SetFlags(vk::ImageUsageFlagBits::eSampled)
        .SetFormat(vk::Format::eR8G8B8A8Unorm)
        .SetData(data)
        .SetName("Fallback texture");

    _fallbackImage = _graphicsResources->ImageResourceManager().Create(imageData);
}

GraphicsContext::~GraphicsContext()
{
    _vulkanContext->Device().destroy(_bindlessLayout);
    _vulkanContext->Device().destroy(_bindlessPool);

    _graphicsResources.reset();
    _vulkanContext.reset();
}

void GraphicsContext::CreateBindlessDescriptorSet()
{
    std::array<vk::DescriptorPoolSize, 6> poolSizes = {
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, MAX_BINDLESS_RESOURCES },
        vk::DescriptorPoolSize { vk::DescriptorType::eStorageBuffer, 1 },
    };

    vk::DescriptorPoolCreateInfo poolCreateInfo {};
    poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind;
    poolCreateInfo.maxSets = MAX_BINDLESS_RESOURCES * poolSizes.size();
    poolCreateInfo.poolSizeCount = poolSizes.size();
    poolCreateInfo.pPoolSizes = poolSizes.data();
    util::VK_ASSERT(_vulkanContext->Device().createDescriptorPool(&poolCreateInfo, nullptr, &_bindlessPool), "Failed creating bindless pool!");

    std::vector<vk::DescriptorSetLayoutBinding> bindings(3);
    vk::DescriptorSetLayoutBinding& combinedImageSampler = bindings[0];
    combinedImageSampler.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    combinedImageSampler.descriptorCount = MAX_BINDLESS_RESOURCES;
    combinedImageSampler.binding = static_cast<uint32_t>(BindlessBinding::eImage);
    combinedImageSampler.stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute;

    vk::DescriptorSetLayoutBinding& storageImage = bindings[1];
    storageImage.descriptorType = vk::DescriptorType::eStorageImage;
    storageImage.descriptorCount = MAX_BINDLESS_RESOURCES;
    storageImage.binding = static_cast<uint32_t>(BindlessBinding::eStorageImage);
    storageImage.stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute;

    vk::DescriptorSetLayoutBinding& materialBinding = bindings[2];
    materialBinding.descriptorType = vk::DescriptorType::eStorageBuffer;
    materialBinding.descriptorCount = 1;
    materialBinding.binding = static_cast<uint32_t>(BindlessBinding::eStorageBuffer);
    materialBinding.stageFlags = vk::ShaderStageFlagBits::eAllGraphics | vk::ShaderStageFlagBits::eCompute;

    vk::StructureChain<vk::DescriptorSetLayoutCreateInfo, vk::DescriptorSetLayoutBindingFlagsCreateInfo> structureChain;

    auto& layoutCreateInfo = structureChain.get<vk::DescriptorSetLayoutCreateInfo>();
    layoutCreateInfo.bindingCount = bindings.size();
    layoutCreateInfo.pBindings = bindings.data();
    layoutCreateInfo.flags = vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;

    std::array<vk::DescriptorBindingFlagsEXT, 2> bindingFlags = {
        vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind,
        vk::DescriptorBindingFlagBits::ePartiallyBound | vk::DescriptorBindingFlagBits::eUpdateAfterBind
    };

    auto& extInfo = structureChain.get<vk::DescriptorSetLayoutBindingFlagsCreateInfoEXT>();
    extInfo.bindingCount = bindings.size();
    extInfo.pBindingFlags = bindingFlags.data();

    std::vector<std::string_view> names { "bindless_color_textures", "bindless_storage_image_r16f", "Materials" };

    _bindlessLayout = PipelineBuilder::CacheDescriptorSetLayout(*_vulkanContext, bindings, names, layoutCreateInfo);

    vk::DescriptorSetAllocateInfo allocInfo {};
    allocInfo.descriptorPool = _bindlessPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &_bindlessLayout;

    util::VK_ASSERT(_vulkanContext->Device().allocateDescriptorSets(&allocInfo, &_bindlessSet), "Failed creating bindless descriptor set!");

    util::NameObject(_bindlessSet, "Bindless DS", _vulkanContext);
}

void GraphicsContext::CreateBindlessMaterialBuffer()
{
    BufferCreation creation {};
    creation.SetSize(MAX_BINDLESS_RESOURCES * sizeof(GPUMaterial::GPUInfo))
        .SetUsageFlags(vk::BufferUsageFlagBits::eStorageBuffer)
        .SetName("Bindless material uniform buffer");

    _bindlessMaterialBuffer = _graphicsResources->BufferResourceManager().Create(creation);
}

void GraphicsContext::UpdateBindlessSet()
{
    UpdateBindlessImages();
    UpdateBindlessMaterials();
}

void GraphicsContext::UpdateBindlessImages()
{
    ImageResourceManager& imageResourceManager { _graphicsResources->ImageResourceManager() };

    for (uint32_t i = 0; i < MAX_BINDLESS_RESOURCES; ++i)
    {
        const GPUImage* image = i < imageResourceManager.Resources().size() && imageResourceManager.Resources()[i].resource.has_value()
            ? &imageResourceManager.Resources()[i].resource.value()
            : imageResourceManager.Access(_fallbackImage);

        // If it can't be sampled, use the fallback.
        if (!(image->flags & vk::ImageUsageFlagBits::eSampled))
        {
            image = imageResourceManager.Access(_fallbackImage);
        }

        if (_sampler.IsNull())
        {
            SamplerCreation createInfo {
                .name = "Graphics context sampler",
                .maxLod = static_cast<float>(image->mips),
            };

            _sampler = _graphicsResources->SamplerResourceManager().Create(createInfo);
        }

        _bindlessImageInfos.at(i).imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        _bindlessImageInfos.at(i).imageView = image->view;
        ResourceHandle<Sampler> samplerHandle = _graphicsResources->SamplerResourceManager().IsValid(image->sampler) ? image->sampler : _sampler;
        _bindlessImageInfos.at(i).sampler = _graphicsResources->SamplerResourceManager().Access(samplerHandle)->sampler;

        _bindlessImageWrites.at(i).dstSet = _bindlessSet;
        _bindlessImageWrites.at(i).dstBinding = static_cast<uint32_t>(BindlessBinding::eImage);
        _bindlessImageWrites.at(i).dstArrayElement = i;
        _bindlessImageWrites.at(i).descriptorType = vk::DescriptorType::eCombinedImageSampler;
        _bindlessImageWrites.at(i).descriptorCount = 1;
        _bindlessImageWrites.at(i).pImageInfo = &_bindlessImageInfos[i];
    }

    _vulkanContext->Device().updateDescriptorSets(MAX_BINDLESS_RESOURCES, _bindlessImageWrites.data(), 0, nullptr);
}

void GraphicsContext::UpdateBindlessMaterials()
{
    MaterialResourceManager& materialResourceManager { _graphicsResources->MaterialResourceManager() };
    BufferResourceManager& bufferResourceManager { _graphicsResources->BufferResourceManager() };

    if (materialResourceManager.Resources().size() == 0)
    {
        return;
    }

    assert(materialResourceManager.Resources().size() < MAX_BINDLESS_RESOURCES && "There are more materials used than the amount that can be stored on the GPU.");

    std::array<GPUMaterial::GPUInfo, MAX_BINDLESS_RESOURCES> materialGPUData;

    for (uint32_t i = 0; i < materialResourceManager.Resources().size(); ++i)
    {
        const GPUMaterial* material = &materialResourceManager.Resources()[i].resource.value();
        materialGPUData[i] = material->gpuInfo;
    }

    const Buffer* buffer = bufferResourceManager.Access(_bindlessMaterialBuffer);
    std::memcpy(buffer->mappedPtr, materialGPUData.data(), materialResourceManager.Resources().size() * sizeof(GPUMaterial::GPUInfo));

    _bindlessMaterialInfo.buffer = buffer->buffer;
    _bindlessMaterialInfo.offset = 0;
    _bindlessMaterialInfo.range = sizeof(GPUMaterial::GPUInfo) * materialResourceManager.Resources().size();

    _bindlessMaterialWrite.dstSet = _bindlessSet;
    _bindlessMaterialWrite.dstBinding = static_cast<uint32_t>(BindlessBinding::eStorageBuffer);
    _bindlessMaterialWrite.dstArrayElement = 0;
    _bindlessMaterialWrite.descriptorType = vk::DescriptorType::eStorageBuffer;
    _bindlessMaterialWrite.descriptorCount = 1;
    _bindlessMaterialWrite.pBufferInfo = &_bindlessMaterialInfo;

    _vulkanContext->Device().updateDescriptorSets(1, &_bindlessMaterialWrite, 0, nullptr);
}
