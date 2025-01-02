#include "build_hzb_pipeline.hpp"

#include "camera_batch.hpp"
#include "gpu_resources.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "math_utils.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <tracy/TracyVulkan.hpp>

BuildHzbPipeline::BuildHzbPipeline(const std::shared_ptr<GraphicsContext>& context, CameraBatch& cameraBatch)
    : _context(context)
    , _cameraBatch(cameraBatch)
{
    CreateSampler();
    CreateDSL();
    CreatPipeline();
    CreateUpdateTemplate();
}

BuildHzbPipeline::~BuildHzbPipeline()
{
    const auto& vkContext = _context->VulkanContext();

    vkContext->Device().destroy(_hzbImageDSL);
    vkContext->Device().destroy(_buildHzbPipelineLayout);
    vkContext->Device().destroy(_buildHzbPipeline);
    vkContext->Device().destroy(_hzbUpdateTemplate);
}

void BuildHzbPipeline::RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Build HZB");
    util::BeginLabel(commandBuffer, "Build HZB", glm::vec3 { 1.0f }, _context->VulkanContext()->Dldi());

    const auto& imageResourceManager = _context->Resources()->ImageResourceManager();
    const auto* hzb = imageResourceManager.Access(_cameraBatch.HZBImage());
    const auto* depth = imageResourceManager.Access(_cameraBatch.DepthImage());

    for (size_t i = 0; i < hzb->mips; ++i)
    {
        uint32_t mipSize = hzb->width >> i;

        vk::ImageView inputTexture = i == 0 ? depth->view : hzb->layerViews[0].mipViews[i - 1];
        vk::ImageView outputTexture = hzb->layerViews[0].mipViews[i];

        // TODO: perhaps not needed
        // util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1, i, 1);

        vk::DescriptorImageInfo inputImageInfo {
            .imageView = inputTexture,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };
        vk::DescriptorImageInfo outputImageInfo {
            .imageView = outputTexture,
            .imageLayout = vk::ImageLayout::eGeneral,
        };

        commandBuffer.pushDescriptorSetWithTemplateKHR<std::array<vk::DescriptorImageInfo, 2>>(_hzbUpdateTemplate, _buildHzbPipelineLayout, 0, { inputImageInfo, outputImageInfo }, _context->VulkanContext()->Dldi());

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _buildHzbPipeline);

        commandBuffer.pushConstants<uint32_t>(_buildHzbPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, { mipSize });

        uint32_t groupSize = DivideRoundingUp(mipSize, 32);
        commandBuffer.dispatch(groupSize, groupSize, 1);

        util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal, 1, i, 1);
    }

    util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral, 1, 0, hzb->mips);

    util::EndLabel(commandBuffer, _context->VulkanContext()->Dldi());
}

void BuildHzbPipeline::CreateSampler()
{
    auto& samplerResourceManager = _context->Resources()->SamplerResourceManager();
    SamplerCreation samplerCreation {
        .name = "HZB Sampler",
        .minFilter = vk::Filter::eLinear,
        .magFilter = vk::Filter::eLinear,
        .anisotropyEnable = false,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .reductionMode = vk::SamplerReductionMode::eMin,
    };
    samplerCreation.SetGlobalAddressMode(vk::SamplerAddressMode::eClampToEdge);

    _hzbSampler = samplerResourceManager.Create(samplerCreation);
}

void BuildHzbPipeline::CreateDSL()
{
    const auto& samplerResourceManager = _context->Resources()->SamplerResourceManager();
    std::vector<vk::DescriptorSetLayoutBinding> bindings(2);
    bindings[0] = {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics,
        .pImmutableSamplers = &samplerResourceManager.Access(_hzbSampler)->sampler,

    };
    bindings[1] = {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics,
    };
    std::vector<std::string_view> names { "inputTexture", "outputTexture" };
    vk::DescriptorSetLayoutCreateInfo dslCreateInfo = vk::DescriptorSetLayoutCreateInfo {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    _hzbImageDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names, dslCreateInfo);
}

void BuildHzbPipeline::CreatPipeline()
{
    std::vector<std::byte> compSpv = shader::ReadFile("shaders/bin/downsample_hzb.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, compSpv);
    auto result = pipelineBuilder.BuildPipeline();

    _buildHzbPipelineLayout = std::get<0>(result);
    _buildHzbPipeline = std::get<1>(result);
}

void BuildHzbPipeline::CreateUpdateTemplate()
{
    std::array<vk::DescriptorUpdateTemplateEntry, 2> updateTemplateEntries {
        vk::DescriptorUpdateTemplateEntry {
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .offset = 0,
            .stride = sizeof(vk::DescriptorImageInfo),
        },
        vk::DescriptorUpdateTemplateEntry {
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
            .offset = sizeof(vk::DescriptorImageInfo),
            .stride = sizeof(vk::DescriptorImageInfo),
        }
    };

    vk::DescriptorUpdateTemplateCreateInfo updateTemplateInfo {
        .descriptorUpdateEntryCount = updateTemplateEntries.size(),
        .pDescriptorUpdateEntries = updateTemplateEntries.data(),
        .templateType = vk::DescriptorUpdateTemplateType::ePushDescriptorsKHR,
        .descriptorSetLayout = _hzbImageDSL,
        .pipelineBindPoint = vk::PipelineBindPoint::eCompute,
        .pipelineLayout = _buildHzbPipelineLayout,
        .set = 0
    };

    _hzbUpdateTemplate = _context->VulkanContext()->Device().createDescriptorUpdateTemplate(updateTemplateInfo);
}
