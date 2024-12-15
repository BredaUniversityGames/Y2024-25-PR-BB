#pragma once

#include "common.hpp"

#include "graphics_context.hpp"
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
    PipelineBuilder(const std::shared_ptr<GraphicsContext>& context);
    virtual ~PipelineBuilder();

    NON_COPYABLE(PipelineBuilder);
    NON_MOVABLE(PipelineBuilder);

    void AddShaderStage(vk::ShaderStageFlagBits stage, const std::vector<std::byte>& spirvBytes, std::string_view entryPoint = "main");
    virtual void BuildPipeline(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout); // TODO: Change output to tuple.

    static vk::DescriptorSetLayout CacheDescriptorSetLayout(const VulkanContext& context, const std::vector<vk::DescriptorSetLayoutBinding>& bindings, const std::vector<std::string_view>& names);

    const std::vector<vk::DescriptorSetLayout>& GetDescriptorSetLayouts() const { return _descriptorSetLayouts; }
    const std::vector<vk::PushConstantRange>& GetPushConstantRanges() const { return _pushConstantRanges; }

protected: // TODO: Review access modifier, right now everything is protected.
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

    void ReflectShaders();
    virtual void ReflectShader(MAYBE_UNUSED const ShaderStage& shaderStage) { }
    void ReflectPushConstants(const ShaderStage& shaderStage);
    void ReflectDescriptorLayouts(const ShaderStage& shaderStage);
    void CreatePipelineLayout(vk::PipelineLayout& pipelineLayout);
    virtual void CreatePipeline(vk::Pipeline& pipeline) = 0;

    vk::ShaderModule CreateShaderModule(const std::vector<std::byte>& spirvBytes);

    static size_t HashBindings(const std::vector<vk::DescriptorSetLayoutBinding>& bindings, const std::vector<std::string_view>& names);
};

class GraphicsPipelineBuilder : public PipelineBuilder
{
public:
    GraphicsPipelineBuilder(const std::shared_ptr<GraphicsContext>& context);
    ~GraphicsPipelineBuilder() override;

    void BuildPipeline(vk::Pipeline& pipeline, vk::PipelineLayout& pipelineLayout) override;

    GraphicsPipelineBuilder& SetInputAssemblyState(const vk::PipelineInputAssemblyStateCreateInfo& createInfo)
    {
        _inputAssemblyStateCreateInfo = createInfo;
        return *this;
    }
    GraphicsPipelineBuilder& SetViewportState(const vk::PipelineViewportStateCreateInfo& createInfo)
    {
        _viewportStateCreateInfo = createInfo;
        return *this;
    }
    GraphicsPipelineBuilder& SetRasterizationState(const vk::PipelineRasterizationStateCreateInfo& createInfo)
    {
        _rasterizationStateCreateInfo = createInfo;
        return *this;
    }
    GraphicsPipelineBuilder& SetMultisampleState(const vk::PipelineMultisampleStateCreateInfo& createInfo)
    {
        _multisampleStateCreateInfo = createInfo;
        return *this;
    }
    GraphicsPipelineBuilder& SetColorBlendState(const vk::PipelineColorBlendStateCreateInfo& createInfo)
    {
        _colorBlendStateCreateInfo = createInfo;
        return *this;
    }
    GraphicsPipelineBuilder& SetDepthStencilState(const vk::PipelineDepthStencilStateCreateInfo& createInfo)
    {
        _depthStencilStateCreateInfo = createInfo;
        return *this;
    }
    GraphicsPipelineBuilder& SetDynamicState(const vk::PipelineDynamicStateCreateInfo& createInfo)
    {
        _dynamicStateCreateInfo = createInfo;
        return *this;
    }

    GraphicsPipelineBuilder& SetColorAttachmentFormats(const std::vector<vk::Format>& formats)
    {
        _colorAttachmentFormats = formats;
        return *this;
    }
    GraphicsPipelineBuilder& SetDepthAttachmentFormat(vk::Format format)
    {
        _depthFormat = format;
        return *this;
    }

    const std::vector<vk::VertexInputAttributeDescription>& GetVertexAttributeDescriptions() const { return _attributeDescriptions; }
    const std::vector<vk::VertexInputBindingDescription>& GetVertexBindingDescriptions() const { return _bindingDescriptions; }

protected:
    void ReflectShader(const ShaderStage& shaderStage) override;
    void CreatePipeline(vk::Pipeline& pipeline) override;

private:
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

    void ReflectVertexInput(const ShaderStage& shaderStage);
};
