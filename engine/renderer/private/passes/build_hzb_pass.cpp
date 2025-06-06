#include "passes/build_hzb_pass.hpp"

#include "camera_batch.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "math_util.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <tracy/TracyVulkan.hpp>

BuildHzbPass::BuildHzbPass(const std::shared_ptr<GraphicsContext>& context, CameraBatch& cameraBatch, vk::DescriptorSetLayout hzbImageDSL)
    : _context(context)
    , _cameraBatch(cameraBatch)
    , _hzbImageDSL(hzbImageDSL)
{
    CreateSampler();
    CreatPipeline();
    CreateUpdateTemplate();
}

BuildHzbPass::~BuildHzbPass()
{
    const auto& vkContext = _context->VulkanContext();

    vkContext->Device().destroy(_buildHzbPipelineLayout);
    vkContext->Device().destroy(_buildHzbPipeline);
    vkContext->Device().destroy(_hzbUpdateTemplate);
}

void BuildHzbPass::RecordCommands(vk::CommandBuffer commandBuffer, MAYBE_UNUSED uint32_t currentFrame, MAYBE_UNUSED const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Build HZB");

    const auto& imageResourceManager = _context->Resources()->ImageResourceManager();
    const auto* hzb = imageResourceManager.Access(_cameraBatch.HZBImage());
    const auto* depth = imageResourceManager.Access(_cameraBatch.DepthImage());

    for (size_t i = 0; i < hzb->mips; ++i)
    {
        uint32_t mipSize = hzb->width >> i;

        vk::ImageView inputTexture = i == 0 ? depth->view : hzb->layerViews[0].mipViews[i - 1];
        vk::ImageView outputTexture = hzb->layerViews[0].mipViews[i];

        util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1, i, 1);

        vk::DescriptorImageInfo inputImageInfo {
            .sampler = _context->Resources()->SamplerResourceManager().Access(_hzbSampler)->sampler,
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

        uint32_t groupSize = math::DivideRoundingUp(mipSize, 16);
        commandBuffer.dispatch(groupSize, groupSize, 1);

        util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal, 1, i, 1);
    }

    util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eGeneral, 1, 0, hzb->mips);
}

void BuildHzbPass::CreateSampler()
{
    auto& samplerResourceManager = _context->Resources()->SamplerResourceManager();
    SamplerCreation samplerCreation {
        .name = "HZB Sampler",
        .minFilter = vk::Filter::eLinear,
        .magFilter = vk::Filter::eLinear,
        .anisotropyEnable = false,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .reductionMode = _cameraBatch.Camera().UsesReverseZ() ? vk::SamplerReductionMode::eMin : vk::SamplerReductionMode::eMax,
    };
    samplerCreation.SetGlobalAddressMode(vk::SamplerAddressMode::eClampToEdge);

    _hzbSampler = samplerResourceManager.Create(samplerCreation);
}

void BuildHzbPass::CreatPipeline()
{
    std::vector<std::byte> compSpv = shader::ReadFile("shaders/bin/downsample_hzb.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, compSpv);
    auto result = pipelineBuilder.BuildPipeline();

    _buildHzbPipelineLayout = std::get<0>(result);
    _buildHzbPipeline = std::get<1>(result);
}

void BuildHzbPass::CreateUpdateTemplate()
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
