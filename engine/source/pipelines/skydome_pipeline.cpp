#include "pipelines/skydome_pipeline.hpp"
#include "shaders/shader_loader.hpp"
#include "single_time_commands.hpp"
#include "vulkan_helper.hpp"

SkydomePipeline::SkydomePipeline(const VulkanBrain& brain, MeshPrimitiveHandle&& sphere, const CameraStructure& camera, ResourceHandle<Image> hdrTarget, ResourceHandle<Image> environmentMap) :
    _brain(brain),
    _camera(camera),
    _hdrTarget(hdrTarget),
    _environmentMap(environmentMap),
    _sphere(sphere)
{
    _sampler = util::CreateSampler(_brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eRepeat, vk::SamplerMipmapMode::eLinear, 0);

    CreateDescriptorSetLayout();
    CreateDescriptorSet();
    CreatePipeline();
}

SkydomePipeline::~SkydomePipeline()
{
    vmaDestroyBuffer(_brain.vmaAllocator, _sphere.vertexBuffer, _sphere.vertexBufferAllocation);
    vmaDestroyBuffer(_brain.vmaAllocator, _sphere.indexBuffer, _sphere.indexBufferAllocation);

    _brain.device.destroy(_descriptorSetLayout);
    _brain.device.destroy(_pipelineLayout);
    _brain.device.destroy(_pipeline);
}

void SkydomePipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame)
{
    vk::RenderingAttachmentInfoKHR finalColorAttachmentInfo{};
    finalColorAttachmentInfo.imageView = _brain.ImageResourceManager().Access(_hdrTarget)->views[0];
    finalColorAttachmentInfo.imageLayout = vk::ImageLayout::eAttachmentOptimalKHR;
    finalColorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    finalColorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    finalColorAttachmentInfo.clearValue.color = vk::ClearColorValue{ 0.0f, 0.0f, 0.0f, 0.0f };

    vk::RenderingInfoKHR renderingInfo{};
    renderingInfo.renderArea.extent = vk::Extent2D{ _brain.ImageResourceManager().Access(_hdrTarget)->width, _brain.ImageResourceManager().Access(_hdrTarget)->height };
    renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &finalColorAttachmentInfo;
    renderingInfo.layerCount = 1;
    renderingInfo.pDepthAttachment = nullptr;
    renderingInfo.pStencilAttachment = nullptr;

    util::BeginLabel(commandBuffer, "Skydome pass", glm::vec3{ 17.0f, 138.0f, 178.0f } / 255.0f, _brain.dldi);
    commandBuffer.beginRenderingKHR(&renderingInfo, _brain.dldi);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 1, 1, &_camera.descriptorSets[currentFrame], 0, nullptr);

    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, &_sphere.vertexBuffer, offsets);
    commandBuffer.bindIndexBuffer(_sphere.indexBuffer, 0, _sphere.indexType);

    commandBuffer.drawIndexed(_sphere.indexCount, 1, 0, 0, 0);

    commandBuffer.endRenderingKHR(_brain.dldi);
    util::EndLabel(commandBuffer, _brain.dldi);
}

void SkydomePipeline::CreatePipeline()
{
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};

    std::array<vk::DescriptorSetLayout, 2> descriptorSets = { _descriptorSetLayout, _camera.descriptorSetLayout };
    pipelineLayoutCreateInfo.setLayoutCount = descriptorSets.size();
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSets.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    util::VK_ASSERT(_brain.device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &_pipelineLayout),
                    "Failed creating geometry pipeline layout!");

    auto vertByteCode = shader::ReadFile("shaders/skydome-v.spv");
    auto fragByteCode = shader::ReadFile("shaders/skydome-f.spv");

    vk::ShaderModule vertModule = shader::CreateShaderModule(vertByteCode, _brain.device);
    vk::ShaderModule fragModule = shader::CreateShaderModule(fragByteCode, _brain.device);

    vk::PipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
    vertShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageCreateInfo.module = vertModule;
    vertShaderStageCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
    fragShaderStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageCreateInfo.module = fragModule;
    fragShaderStageCreateInfo.pName = "main";

    // TODO: This shader stuff can be moved into a util function for brevity.
    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

    auto bindingDesc = Vertex::GetBindingDescription();
    auto attributes = Vertex::GetAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = attributes.size();
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributes.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
    inputAssemblyStateCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = vk::False;

    std::array<vk::DynamicState, 2> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
    dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.scissorCount = 1;

    vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
    rasterizationStateCreateInfo.depthClampEnable = vk::False;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = vk::False;
    rasterizationStateCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizationStateCreateInfo.lineWidth = 1.0f;
    rasterizationStateCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizationStateCreateInfo.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizationStateCreateInfo.depthBiasEnable = vk::False;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

    vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
    multisampleStateCreateInfo.sampleShadingEnable = vk::False;
    multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampleStateCreateInfo.minSampleShading = 1.0f;
    multisampleStateCreateInfo.pSampleMask = nullptr;
    multisampleStateCreateInfo.alphaToCoverageEnable = vk::False;
    multisampleStateCreateInfo.alphaToOneEnable = vk::False;

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.blendEnable = vk::False;
    colorBlendAttachmentState.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
    colorBlendStateCreateInfo.logicOpEnable = vk::False;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    vk::PipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
    depthStencilStateCreateInfo.depthTestEnable = false;
    depthStencilStateCreateInfo.depthWriteEnable = false;

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStages;
    pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.layout = _pipelineLayout;
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.basePipelineHandle = nullptr;
    pipelineCreateInfo.basePipelineIndex = -1;

    vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfoKhr{};
    pipelineRenderingCreateInfoKhr.colorAttachmentCount = 1;
    pipelineRenderingCreateInfoKhr.pColorAttachmentFormats = &_brain.ImageResourceManager().Access(_hdrTarget)->format;

    pipelineCreateInfo.pNext = &pipelineRenderingCreateInfoKhr;
    pipelineCreateInfo.renderPass = nullptr; // Using dynamic rendering.

    auto result = _brain.device.createGraphicsPipeline(nullptr, pipelineCreateInfo, nullptr);
    util::VK_ASSERT(result.result, "Failed creating the skydome pipeline layout!");
    _pipeline = result.value;

    _brain.device.destroy(vertModule);
    _brain.device.destroy(fragModule);
}

void SkydomePipeline::CreateDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 1> bindings{};

    vk::DescriptorSetLayoutBinding& samplerLayoutBinding{bindings[0]};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo createInfo{};
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    util::VK_ASSERT(_brain.device.createDescriptorSetLayout(&createInfo, nullptr, &_descriptorSetLayout),
                    "Failed creating skydome descriptor set layout!");
}

void SkydomePipeline::CreateDescriptorSet()
{
    vk::DescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.descriptorPool = _brain.descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &_descriptorSetLayout;

    util::VK_ASSERT(_brain.device.allocateDescriptorSets(&allocateInfo, &_descriptorSet),
                    "Failed allocating descriptor sets!");

    vk::DescriptorImageInfo imageInfo{};
    imageInfo.sampler = *_sampler;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = _brain.ImageResourceManager().Access(_environmentMap)->views[0];

    vk::WriteDescriptorSet descriptorWrite{};
    descriptorWrite.dstSet = _descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    _brain.device.updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
}
