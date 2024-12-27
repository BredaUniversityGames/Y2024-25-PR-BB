#include "pipelines/geometry_pipeline.hpp"

#include "batch_buffer.hpp"
#include "components/transform_helpers.hpp"
#include "components/world_matrix_component.hpp"
#include "constants.hpp"
#include "ecs_module.hpp"
#include "gpu_scene.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "math_utils.hpp"
#include "pipeline_builder.hpp"
#include "resource_management/buffer_resource_manager.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "shaders/shader_loader.hpp"
#include "vertex.hpp"
#include "vulkan_context.hpp"
#include "vulkan_helper.hpp"

#include <entt/entt.hpp>

GeometryPipeline::GeometryPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers, ResourceHandle<GPUImage> hzbImage, const GPUScene& gpuScene)
    : _context(context)
    , _gBuffers(gBuffers)
    , _hzbImage(hzbImage)
{
    CreateStaticPipeline();
    CreateSkinnedPipeline();
    CreatBuildHzbPipeline();

    auto mainDrawBufferHandle = gpuScene.IndirectDrawBuffer(0);
    const auto* mainDrawBuffer = _context->Resources()->BufferResourceManager().Access(mainDrawBufferHandle);

    BufferCreation creation {
        .size = mainDrawBuffer->size,
        .usage = mainDrawBuffer->usage,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .name = "Geometry draw buffer",
    };

    _drawBuffer = _context->Resources()->BufferResourceManager().Create(creation);

    CreateDrawBufferDescriptorSet(gpuScene);

    BufferCreation visibilityCreation {
        .size = sizeof(bool) * MAX_INSTANCES, // TODO: Can be packed into multiple bits
        .usage = vk::BufferUsageFlagBits::eStorageBuffer,
        .isMappable = false,
        .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
        .name = "Visibility Buffer"
    };
    _visibilityBuffer = _context->Resources()->BufferResourceManager().Create(visibilityCreation);

    std::vector<vk::DescriptorSetLayoutBinding> bindings {
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics }
    };
    std::vector<std::string_view> names { "VisibilityBuffer" };

    _visibilityDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names);
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &_visibilityDSL,
    };
    _visibilityDescriptorSet = _context->VulkanContext()->Device().allocateDescriptorSets(allocateInfo).front();

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_visibilityBuffer);

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = buffer->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _visibilityDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets({ bufferWrite }, {});

    _culler = std::make_unique<IndirectCuller>(_context, gpuScene.MainCamera(), _drawBuffer, _drawBufferDescriptorSet, _visibilityBuffer, _visibilityDescriptorSet);
}

GeometryPipeline::~GeometryPipeline()
{
    _context->VulkanContext()->Device().destroy(_staticPipeline);
    _context->VulkanContext()->Device().destroy(_staticPipelineLayout);
    _context->VulkanContext()->Device().destroy(_skinnedPipeline);
    _context->VulkanContext()->Device().destroy(_skinnedPipelineLayout);
}

void GeometryPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Geometry Pipeline");
    _culler->RecordCommands(commandBuffer, currentFrame, scene, true);

    std::array<vk::RenderingAttachmentInfoKHR, DEFERRED_ATTACHMENT_COUNT> colorAttachmentInfos {};
    for (size_t i = 0; i < colorAttachmentInfos.size(); ++i)
    {
        vk::RenderingAttachmentInfoKHR& info { colorAttachmentInfos.at(i) };
        info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        info.storeOp = vk::AttachmentStoreOp::eStore;
        info.loadOp = vk::AttachmentLoadOp::eClear;
        info.clearValue.color = vk::ClearColorValue { .float32 = { { 0.0f, 0.0f, 0.0f, 0.0f } } };
    }

    const ImageResourceManager& imageResourceManager = _context->Resources()->ImageResourceManager();
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
    {
        colorAttachmentInfos.at(i).imageView = imageResourceManager.Access(_gBuffers.Attachments().at(i))->view;
    }

    vk::RenderingAttachmentInfoKHR depthAttachmentInfo {};
    depthAttachmentInfo.imageView = imageResourceManager.Access(_gBuffers.Depth())->view;
    depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue { 0.0f, 0 };

    vk::RenderingInfoKHR renderingInfo {};
    glm::uvec2 displaySize = _gBuffers.Size();
    renderingInfo.renderArea.extent = vk::Extent2D { displaySize.x, displaySize.y };
    renderingInfo.renderArea.offset = vk::Offset2D { 0, 0 };
    renderingInfo.colorAttachmentCount = colorAttachmentInfos.size();
    renderingInfo.pColorAttachments = colorAttachmentInfos.data();
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    renderingInfo.pStencilAttachment = nullptr;

    commandBuffer.beginRenderingKHR(&renderingInfo, _context->VulkanContext()->Dldi());
    if (scene.gpuScene->StaticDrawRange().count > 0)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _staticPipeline);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 1, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _staticPipelineLayout, 2, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});

        vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->VertexBuffer())->buffer;
        vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.staticBatchBuffer->IndexBuffer())->buffer;
        vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer)->buffer;

        commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
        commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.staticBatchBuffer->IndexType());
        commandBuffer.drawIndexedIndirect(
            indirectDrawBuffer,
            scene.gpuScene->StaticDrawRange().start * sizeof(DrawIndexedIndirectCommand),
            scene.gpuScene->StaticDrawRange().count,
            sizeof(DrawIndexedIndirectCommand),
            _context->VulkanContext()->Dldi());

        _context->GetDrawStats().IndirectDraw(scene.gpuScene->StaticDrawRange().count, scene.gpuScene->DrawCommandIndexCount(scene.gpuScene->StaticDrawRange()));
    }

    if (scene.gpuScene->SkinnedDrawRange().count > 0)
    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _skinnedPipeline);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 0, { _context->BindlessSet() }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 1, { scene.gpuScene->GetObjectInstancesDescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 2, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _skinnedPipelineLayout, 3, { scene.gpuScene->GetSkinDescriptorSet(currentFrame) }, {});

        vk::Buffer vertexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->VertexBuffer())->buffer;
        vk::Buffer indexBuffer = _context->Resources()->BufferResourceManager().Access(scene.skinnedBatchBuffer->IndexBuffer())->buffer;
        vk::Buffer indirectDrawBuffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer)->buffer;

        commandBuffer.pushConstants<uint32_t>(_skinnedPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, { scene.gpuScene->SkinnedDrawRange().start });

        commandBuffer.bindVertexBuffers(0, { vertexBuffer }, { 0 });
        commandBuffer.bindIndexBuffer(indexBuffer, 0, scene.skinnedBatchBuffer->IndexType());
        commandBuffer.drawIndexedIndirect(
            indirectDrawBuffer,
            scene.gpuScene->SkinnedDrawRange().start * sizeof(DrawIndexedIndirectCommand),
            scene.gpuScene->SkinnedDrawRange().count,
            sizeof(DrawIndexedIndirectCommand),
            _context->VulkanContext()->Dldi());

        _context->GetDrawStats().IndirectDraw(scene.gpuScene->SkinnedDrawRange().count, scene.gpuScene->DrawCommandIndexCount(scene.gpuScene->SkinnedDrawRange()));
    }

    commandBuffer.endRenderingKHR(_context->VulkanContext()->Dldi());

    BuildHzb(scene, commandBuffer);
}

void GeometryPipeline::CreateStaticPipeline()
{
    std::array<vk::PipelineColorBlendAttachmentState, DEFERRED_ATTACHMENT_COUNT> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eGreater;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    std::vector<vk::Format> formats(DEFERRED_ATTACHMENT_COUNT);
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
        formats.at(i) = _context->Resources()->ImageResourceManager().Access(_gBuffers.Attachments().at(i))->format;

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/geom.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/geom.frag.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetDepthStencilState(depthStencilStateCreateInfo)
                      .SetColorAttachmentFormats(formats)
                      .SetDepthAttachmentFormat(_gBuffers.DepthFormat())
                      .BuildPipeline();

    _staticPipelineLayout = std::get<0>(result);
    _staticPipeline = std::get<1>(result);
}

void GeometryPipeline::CreateSkinnedPipeline()
{
    std::array<vk::PipelineColorBlendAttachmentState, DEFERRED_ATTACHMENT_COUNT> colorBlendAttachmentStates {};
    for (auto& blendAttachmentState : colorBlendAttachmentStates)
    {
        blendAttachmentState.blendEnable = vk::False;
        blendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    }

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = colorBlendAttachmentStates.size();
    colorBlendStateCreateInfo.pAttachments = colorBlendAttachmentStates.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {};
    depthStencilStateCreateInfo.depthTestEnable = true;
    depthStencilStateCreateInfo.depthWriteEnable = true;
    depthStencilStateCreateInfo.depthCompareOp = vk::CompareOp::eGreaterOrEqual;
    depthStencilStateCreateInfo.depthBoundsTestEnable = false;
    depthStencilStateCreateInfo.minDepthBounds = 0.0f;
    depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
    depthStencilStateCreateInfo.stencilTestEnable = false;

    const ImageResourceManager& imageResourceManager = _context->Resources()->ImageResourceManager();

    std::vector<vk::Format> formats(DEFERRED_ATTACHMENT_COUNT);
    for (size_t i = 0; i < DEFERRED_ATTACHMENT_COUNT; ++i)
    {
        formats.at(i) = imageResourceManager.Access(_gBuffers.Attachments().at(i))->format;
    }

    std::vector<std::byte> vertSpv = shader::ReadFile("shaders/bin/skinned_geom.vert.spv");
    std::vector<std::byte> fragSpv = shader::ReadFile("shaders/bin/geom.frag.spv");

    GraphicsPipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eVertex, vertSpv);
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eFragment, fragSpv);
    auto result = pipelineBuilder
                      .SetColorBlendState(colorBlendStateCreateInfo)
                      .SetDepthStencilState(depthStencilStateCreateInfo)
                      .SetColorAttachmentFormats(formats)
                      .SetDepthAttachmentFormat(_gBuffers.DepthFormat())
                      .BuildPipeline();

    _skinnedPipelineLayout = std::get<0>(result);
    _skinnedPipeline = std::get<1>(result);
}

void GeometryPipeline::CreatBuildHzbPipeline()
{
    const auto& samplerResourceManager = _context->Resources()->SamplerResourceManager();
    std::vector<vk::DescriptorSetLayoutBinding> bindings(2);
    bindings[0] = {
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics,
        .pImmutableSamplers = &samplerResourceManager.Access(samplerResourceManager.GetDefaultSamplerHandle())->sampler,

    };
    bindings[1] = {
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .stageFlags = vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eAllGraphics,
    };
    std::vector<std::string_view> names { "inputTexture", "outputTexture" };
    vk::DescriptorSetLayoutCreateInfo dslCreateInfo = vk::DescriptorSetLayoutCreateInfo {
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR,
    };

    _hzbImageDSL = PipelineBuilder::CacheDescriptorSetLayout(*_context->VulkanContext(), bindings, names, dslCreateInfo);

    std::vector<std::byte> compSpv = shader::ReadFile("shaders/bin/downsample_hzb.comp.spv");

    ComputePipelineBuilder pipelineBuilder { _context };
    pipelineBuilder.AddShaderStage(vk::ShaderStageFlagBits::eCompute, compSpv);
    auto result = pipelineBuilder.BuildPipeline();

    _buildHzbPipelineLayout = std::get<0>(result);
    _buildHzbPipeline = std::get<1>(result);

    std::array<vk::DescriptorUpdateTemplateEntry, 2> updateTemplateEntries {
        vk::DescriptorUpdateTemplateEntry {
            .offset = 0,
            .stride = sizeof(vk::DescriptorImageInfo),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        },
        vk::DescriptorUpdateTemplateEntry {
            .offset = sizeof(vk::DescriptorImageInfo),
            .stride = sizeof(vk::DescriptorImageInfo),
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageImage,
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

void GeometryPipeline::CreateDrawBufferDescriptorSet(const GPUScene& gpuScene)
{
    vk::DescriptorSetLayout layout = gpuScene.DrawBufferLayout();
    vk::DescriptorSetAllocateInfo allocateInfo {
        .descriptorPool = _context->VulkanContext()->DescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    _drawBufferDescriptorSet = _context->VulkanContext()->Device().allocateDescriptorSets(allocateInfo).front();

    const Buffer* buffer = _context->Resources()->BufferResourceManager().Access(_drawBuffer);

    vk::DescriptorBufferInfo bufferInfo {
        .buffer = buffer->buffer,
        .offset = 0,
        .range = vk::WholeSize,
    };

    vk::WriteDescriptorSet bufferWrite {
        .dstSet = _drawBufferDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .pBufferInfo = &bufferInfo,
    };

    _context->VulkanContext()->Device().updateDescriptorSets({ bufferWrite }, {});
}

void GeometryPipeline::BuildHzb(const RenderSceneDescription& scene, vk::CommandBuffer commandBuffer)
{
    TracyVkZone(scene.tracyContext, commandBuffer, "Build HZB");

    const auto& imageResourceManager = _context->Resources()->ImageResourceManager();
    const auto* hzb = _context->Resources()->ImageResourceManager().Access(_hzbImage);
    const auto* depth = imageResourceManager.Access(_gBuffers.Depth());

    util::TransitionImageLayout(commandBuffer, depth->image, depth->format, vk::ImageLayout::eDepthStencilAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);

    for (size_t i = 0; i < hzb->mips; ++i)
    {
        uint32_t mipSize = hzb->width >> i;

        vk::ImageView inputTexture = depth->view; // i == 0 ? depth->view : hzb->layerViews[0].mipViews[i - 1];
        vk::ImageView outputTexture = hzb->layerViews[0].mipViews[i];

        if (i > 0)
        {
            // util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eGeneral, vk::ImageLayout::eShaderReadOnlyOptimal, 1, i - 1, 1);
        }
        util::TransitionImageLayout(commandBuffer, hzb->image, hzb->format, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1, i, 1);

        vk::DescriptorImageInfo inputImageInfo {
            //.sampler = samplerResourceManager.Access(samplerResourceManager.GetDefaultSamplerHandle())->sampler,
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
    }

    util::TransitionImageLayout(commandBuffer, depth->image, depth->format, vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1, 0, 1, vk::ImageAspectFlagBits::eDepth);
}
