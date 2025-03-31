#include "passes/decal_pass.hpp"

#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <filesystem>
#include <glm/gtx/quaternion.hpp>
#include <tracy/Tracy.hpp>

DecalPass::DecalPass(const std::shared_ptr<GraphicsContext>& context, const Settings::Lighting& lightingSettings, const GBuffers& gBuffers)
    : _context(context)
    , _lightingSettings(lightingSettings)
    , _gBuffers(gBuffers)
    , _screenSize({ _gBuffers.Size().x, _gBuffers.Size().y })
{
    _pushConstants.albedoMIndex = _gBuffers.Attachments()[0].Index();
    _pushConstants.normalRIndex = _gBuffers.Attachments()[1].Index();
    _pushConstants.depthIndex = _gBuffers.Depth().Index();
    _pushConstants.screenSize = _screenSize;

    CreateBuffers();
    CreateDescriptorSetLayouts();
    CreateDescriptorSets();
    CreatePipeline();
}

DecalPass::~DecalPass()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
    _context->VulkanContext()->Device().destroy(_decalDescriptorSetLayout);
    _context->VulkanContext()->Device().destroy(_outputImageDescriptorSetLayout);
}

void DecalPass::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    UpdateDecalBuffer(currentFrame);

    // TODO: sync..?

    TracyVkZone(scene.tracyContext, commandBuffer, "Decals Pass");

    util::BeginLabel(commandBuffer, "Decals pass", glm::vec3 { 230.0f, 230.0f, 250.0f } / 255.0f, _context->VulkanContext()->Dldi());

    vk::ImageView outputTexture = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments()[0])->view;
    vk::DescriptorImageInfo outputImageInfo {
        .imageView = outputTexture,
        .imageLayout = vk::ImageLayout::eGeneral,
    };

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, _context->BindlessSet(), {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 1, scene.gpuScene->MainCamera().DescriptorSet(currentFrame), {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 2, _decalDescriptorSets[currentFrame], {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 3, _outputImageDescriptorSet, {});

    _pushConstants.decalNormalThreshold = glm::cos(_lightingSettings.decalNormalThreshold);
    _pushConstants.decalCount = _decalCount;
    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { _pushConstants });

    const int groupSize = 16;
    glm::uvec2 numGroups = glm::uvec2(_screenSize.x / groupSize, _screenSize.y / groupSize);
    commandBuffer.dispatch(numGroups.x, numGroups.y, _decalCount);

    util::EndLabel(commandBuffer, _context->VulkanContext()->Dldi());
}

void DecalPass::SpawnDecal(glm::vec3 normal, glm::vec3 position, glm::vec2 size, std::string albedoName)
{
    const auto image = GetDecalImage(albedoName);

    glm::vec2 imageSize;
    imageSize.x = _context->Resources()->ImageResourceManager().Access(image)->width;
    imageSize.y = _context->Resources()->ImageResourceManager().Access(image)->height;

    const float decalThickness = 1.f;

    glm::vec3 forward = -normal;
    glm::vec3 up = std::abs(glm::dot(forward, glm::vec3(0.0f, 1.0f, 0.0f))) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 right = glm::normalize(glm::cross(up, forward));
    up = glm::cross(forward, right);
    glm::quat orientation = glm::quat(glm::mat3(right, up, forward));

    const glm::mat4 translationMatrix = glm::translate(glm::mat4 { 1.0f }, position);
    const glm::mat4 rotationMatrix = glm::toMat4(orientation);
    const glm::mat4 scaleMatrix = glm::scale(glm::mat4 { 1.0f }, glm::vec3(imageSize.x * size.x, imageSize.y * size.y, decalThickness));

    DecalData newDecal {
        .invModel = glm::inverse(translationMatrix * rotationMatrix * scaleMatrix),
        .orientation = glm::normalize(normal),
        .albedoIndex = image.Index(),
    };

    // Place a new decal, and fill the buffer
    const uint32_t decalIndex = (_decalCount++) % MAX_DECALS;
    _decals[decalIndex] = newDecal;
}

void DecalPass::ResetDecals()
{
    _decalCount = 0;
}

ResourceHandle<GPUImage>& DecalPass::GetDecalImage(std::string fileName)
{
    auto got = _decalImages.find(fileName);

    if (got == _decalImages.end())
    {
        if (std::filesystem::exists("assets/textures/decals/" + fileName))
        {
            CPUImage creation;
            creation.SetFlags(vk::ImageUsageFlagBits::eSampled);
            creation.SetName(fileName);
            creation.FromPNG("assets/textures/decals/" + fileName);
            creation.isHDR = false;
            auto image = _context->Resources()->ImageResourceManager().Create(creation);
            auto& resource = _decalImages.emplace(fileName, image).first->second;
            _context->UpdateBindlessSet();
            return resource;
        }

        bblog::error("[Error] Decal image not found!");
        return _decalImages.begin()->second;
    }

    return got->second;
}

void DecalPass::UpdateDecalBuffer(uint32_t currentFrame)
{
    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_decalBuffers[currentFrame]);
    std::memcpy(buffer->mappedPtr, _decals.data(), sizeof(DecalData) * MAX_DECALS);
}

void DecalPass::CreatePipeline()
{
    auto vkContext { _context->VulkanContext() };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    std::array<vk::DescriptorSetLayout, 4> layouts = { _context->BindlessLayout(), CameraResource::DescriptorSetLayout(), _decalDescriptorSetLayout, _outputImageDescriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = layouts.size();
    pipelineLayoutCreateInfo.pSetLayouts = layouts.data();

    vk::PushConstantRange pcRange = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(_pushConstants),
    };

    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pcRange;

    _pipelineLayout = vkContext->Device().createPipelineLayout(pipelineLayoutCreateInfo);

    std::vector<std::byte> byteCode = shader::ReadFile("shaders/bin/decals.comp.spv");

    vk::ShaderModule shaderModule = shader::CreateShaderModule(byteCode, vkContext->Device());

    vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .module = shaderModule,
        .pName = "main",
    };

    vk::ComputePipelineCreateInfo computePipelineCreateInfo {
        .stage = shaderStageCreateInfo,
        .layout = _pipelineLayout,
    };

    auto result = vkContext->Device().createComputePipeline(nullptr, computePipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the decals compute pipeline!");
    _pipeline = result.value;

    vkContext->Device().destroy(shaderModule);
}

void DecalPass::CreateDescriptorSetLayouts()
{
    { // Decal UBO
        vk::DescriptorSetLayoutBinding binding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        };

        std::vector<vk::DescriptorSetLayoutBinding> bindings { binding };
        std::vector<std::string_view> names { "DecalUBO" };
        _decalDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
    }

    { // Output Tex
        vk::DescriptorSetLayoutBinding binding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute,
        };

        std::vector<vk::DescriptorSetLayoutBinding> bindings { binding };
        std::vector<std::string_view> names { "outputTexture" };
        _outputImageDescriptorSetLayout = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
    }
}

void DecalPass::CreateDescriptorSets()
{
    { // Decal UBO
        std::array<vk::DescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts = { _decalDescriptorSetLayout, _decalDescriptorSetLayout, _decalDescriptorSetLayout };
        vk::DescriptorSetAllocateInfo allocateInfo {
            .descriptorPool = _context->VulkanContext()->DescriptorPool(),
            .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
            .pSetLayouts = layouts.data(),
        };

        std::array<vk::DescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets;
        util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, descriptorSets.data()),
            "Failed allocating Decal Uniform Buffer descriptor sets!");

        for (size_t i = 0; i < descriptorSets.size(); ++i)
        {
            _decalDescriptorSets[i] = descriptorSets[i];
            UpdateDecalBufferDescriptorSet(i);
        }
    }

    { // Output Tex
        vk::DescriptorSetAllocateInfo allocateInfo {
            .descriptorPool = _context->VulkanContext()->DescriptorPool(),
            .descriptorSetCount = 1,
            .pSetLayouts = &_outputImageDescriptorSetLayout,
        };

        util::VK_ASSERT(_context->VulkanContext()->Device().allocateDescriptorSets(&allocateInfo, &_outputImageDescriptorSet),
            "Failed allocating Decal Uniform Buffer descriptor sets!");

        UpdateOutputImageDescriptorSet();
    }
}

void DecalPass::UpdateDecalBufferDescriptorSet(uint32_t frameIndex)
{
    vk::DescriptorBufferInfo bufferInfo {
        .buffer = _context->Resources()->BufferResourceManager().Access(_decalBuffers[frameIndex])->buffer,
        .offset = 0,
        .range = sizeof(DecalData) * MAX_DECALS,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _decalDescriptorSets[frameIndex],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets(1, &bufferWrite, 0, nullptr);
}

void DecalPass::UpdateOutputImageDescriptorSet()
{
    vk::DescriptorImageInfo imageInfo {
        .sampler = VK_NULL_HANDLE,
        .imageView = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments()[0])->view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal, // TODO: in between texture? can't have color attachment image layout for storage image
    };

    vk::WriteDescriptorSet imageWrite {
        .dstSet = _outputImageDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .pImageInfo = &imageInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets(1, &imageWrite, 0, nullptr);
}

void DecalPass::CreateBuffers()
{
    for (uint32_t i = 0; i < _decalBuffers.size(); ++i)
    {
        BufferCreation createInfo {
            .size = sizeof(DecalData) * MAX_DECALS,
            .usage = vk::BufferUsageFlagBits::eUniformBuffer,
            .isMappable = true,
            .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
            .name = "Decal Buffer"
        };

        _decalBuffers[i] = _context->Resources()->BufferResourceManager().Create(createInfo);

        const Buffer* decalBuffer = _context->Resources()->BufferResourceManager().Access(_decalBuffers[i]);
        std::memcpy(decalBuffer->mappedPtr, &_decals, sizeof(DecalData) * MAX_DECALS);
    }
}