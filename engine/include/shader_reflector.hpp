#pragma once

#include "common.hpp"

class ShaderReflector
{
public:
    ShaderReflector(const VulkanBrain& brain);
    ~ShaderReflector();

    NON_COPYABLE(ShaderReflector);
    NON_MOVABLE(ShaderReflector);

    void AddShaderStage(vk::ShaderStageFlagBits stage, const std::vector<std::byte>& spirvBytes, std::string_view entryPoint = "main");
    vk::Pipeline BuildPipeline();

    void SetInputAssemblyState(const vk::PipelineInputAssemblyStateCreateInfo& createInfo) { _inputAssemblyStateCreateInfo = createInfo; }
    void SetViewportState(const vk::PipelineViewportStateCreateInfo& createInfo) { _viewportStateCreateInfo = createInfo; }
    void SetRasterizationState(const vk::PipelineRasterizationStateCreateInfo& createInfo) { _rasterizationStateCreateInfo = createInfo; }
    void SetMultisampleState(const vk::PipelineMultisampleStateCreateInfo& createInfo) { _multisampleStateCreateInfo = createInfo; }
    void SetColorBlendState(const vk::PipelineColorBlendStateCreateInfo& createInfo) { _colorBlendStateCreateInfo = createInfo; }
    void SetDepthStencilState(const vk::PipelineDepthStencilStateCreateInfo& createInfo) { _depthStencilStateCreateInfo = createInfo; }

    void SetColorAttachmentFormats(const std::vector<vk::Format>& formats) { _colorAttachmentFormats = formats; }
    void SetDepthAttachmentFormat(vk::Format format) { _depthFormat = format; }

    const std::vector<vk::DescriptorSetLayout>& GetDescriptorSetLayouts() const { return _descriptorSetLayouts; }
    const std::vector<vk::PushConstantRange>& GetPushConstantRanges() const { return _pushConstantRanges; }

    const std::vector<vk::VertexInputAttributeDescription>& GetVertexAttributeDescriptions() const { return _attributeDescriptions; }
    const std::vector<vk::VertexInputBindingDescription>& GetVertexBindingDescriptions() const { return _bindingDescriptions; }

private:
    struct ShaderStage
    {
        vk::ShaderStageFlagBits stage;
        std::string_view entryPoint;
        std::vector<std::byte> spirvBytes; // TODO: Consider keeping this a reference to prevent copy.
        SpvReflectShaderModule reflectModule;
    };

    const VulkanBrain& _brain;
    std::vector<vk::PipelineShaderStageCreateInfo> _pipelineShaderStages;
    std::vector<ShaderStage> _shaderStages;
    vk::PipelineLayout _pipelineLayout;
    std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
    std::vector<vk::PushConstantRange> _pushConstantRanges;

    std::vector<vk::VertexInputAttributeDescription> _attributeDescriptions;
    std::vector<vk::VertexInputBindingDescription> _bindingDescriptions;

    std::optional<vk::PipelineInputAssemblyStateCreateInfo> _inputAssemblyStateCreateInfo;
    std::optional<vk::PipelineViewportStateCreateInfo> _viewportStateCreateInfo;
    std::optional<vk::PipelineRasterizationStateCreateInfo> _rasterizationStateCreateInfo;
    std::optional<vk::PipelineMultisampleStateCreateInfo> _multisampleStateCreateInfo;
    std::optional<vk::PipelineColorBlendStateCreateInfo> _colorBlendStateCreateInfo;
    std::optional<vk::PipelineDepthStencilStateCreateInfo> _depthStencilStateCreateInfo;

    std::vector<vk::Format> _colorAttachmentFormats;
    vk::Format _depthFormat;

    void ReflectShaders();
    void ReflectVertexInput(const ShaderStage& shaderStage);
    void ReflectPushConstants(const ShaderStage& shaderStage);
    void ReflectDescriptorLayouts(const ShaderStage& shaderStage);
    void CreatePipelineLayout();
    vk::Pipeline CreatePipeline();

    vk::ShaderModule CreateShaderModule(const std::vector<std::byte>& spirvBytes);
};