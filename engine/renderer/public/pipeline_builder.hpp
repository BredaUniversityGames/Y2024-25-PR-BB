#pragma once

#include "common.hpp"

#include <cstddef>
#include <optional>
#include <spirv_reflect.h>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

class GraphicsContext;
class VulkanContext;

class PipelineBuilder
{
public:
    PipelineBuilder(std::shared_ptr<GraphicsContext> context);
    ~PipelineBuilder();

    NON_COPYABLE(PipelineBuilder);
    NON_MOVABLE(PipelineBuilder);

    PipelineBuilder& AddShaderStage(vk::ShaderStageFlagBits stage, const std::vector<std::byte>& spirvBytes, std::string_view entryPoint = "main");
    void BuildPipeline(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout);

    static vk::DescriptorSetLayout CacheDescriptorSetLayout(const VulkanContext& context, const std::vector<vk::DescriptorSetLayoutBinding>& bindings, const std::vector<std::string_view>& names);

    PipelineBuilder& SetInputAssemblyState(const vk::PipelineInputAssemblyStateCreateInfo& createInfo)
    {
        _inputAssemblyStateCreateInfo = createInfo;
        return *this;
    }
    PipelineBuilder& SetViewportState(const vk::PipelineViewportStateCreateInfo& createInfo)
    {
        _viewportStateCreateInfo = createInfo;
        return *this;
    }
    PipelineBuilder& SetRasterizationState(const vk::PipelineRasterizationStateCreateInfo& createInfo)
    {
        _rasterizationStateCreateInfo = createInfo;
        return *this;
    }
    PipelineBuilder& SetMultisampleState(const vk::PipelineMultisampleStateCreateInfo& createInfo)
    {
        _multisampleStateCreateInfo = createInfo;
        return *this;
    }
    PipelineBuilder& SetColorBlendState(const vk::PipelineColorBlendStateCreateInfo& createInfo)
    {
        _colorBlendStateCreateInfo = createInfo;
        return *this;
    }
    PipelineBuilder& SetDepthStencilState(const vk::PipelineDepthStencilStateCreateInfo& createInfo)
    {
        _depthStencilStateCreateInfo = createInfo;
        return *this;
    }
    PipelineBuilder& SetDynamicState(const vk::PipelineDynamicStateCreateInfo& createInfo)
    {
        _dynamicStateCreateInfo = createInfo;
        return *this;
    }

    PipelineBuilder& SetColorAttachmentFormats(const std::vector<vk::Format>& formats)
    {
        _colorAttachmentFormats = formats;
        return *this;
    }
    PipelineBuilder& SetDepthAttachmentFormat(vk::Format format)
    {
        _depthFormat = format;
        return *this;
    }

    const std::vector<vk::DescriptorSetLayout>& GetDescriptorSetLayouts() const { return _descriptorSetLayouts; }
    const std::vector<vk::PushConstantRange>& GetPushConstantRanges() const { return _pushConstantRanges; }

    const std::vector<vk::VertexInputAttributeDescription>& GetVertexAttributeDescriptions() const { return _attributeDescriptions; }
    const std::vector<vk::VertexInputBindingDescription>& GetVertexBindingDescriptions() const { return _bindingDescriptions; }

private:
    struct ShaderStage
    {
        vk::ShaderStageFlagBits stage;
        std::string_view entryPoint;
        const std::vector<std::byte>& spirvBytes;
        SpvReflectShaderModule reflectModule;
        vk::ShaderModule shaderModule;
    };

    std::shared_ptr<GraphicsContext> _context;
    std::vector<vk::PipelineShaderStageCreateInfo> _pipelineShaderStages;
    std::vector<ShaderStage> _shaderStages;

    vk::Pipeline _pipeline;
    vk::PipelineLayout _pipelineLayout;

    static std::unordered_map<size_t, vk::DescriptorSetLayout> _cacheDescriptorSetLayouts;

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
    std::optional<vk::PipelineDynamicStateCreateInfo> _dynamicStateCreateInfo;

    std::vector<vk::Format> _colorAttachmentFormats;
    vk::Format _depthFormat { vk::Format::eUndefined };

    void ReflectShaders();
    void ReflectVertexInput(const ShaderStage& shaderStage);
    void ReflectPushConstants(const ShaderStage& shaderStage);
    void ReflectDescriptorLayouts(const ShaderStage& shaderStage);
    void CreatePipelineLayout(vk::PipelineLayout& pipelineLayout);
    void CreatePipeline(vk::Pipeline& pipeline);

    vk::ShaderModule CreateShaderModule(const std::vector<std::byte>& spirvBytes);

    static size_t HashBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings, const std::vector<std::string_view>& names);
};