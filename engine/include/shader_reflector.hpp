#pragma once

#include "common.hpp"

class ShaderReflector
{
public:
    ShaderReflector(const VulkanBrain& brain);
    ~ShaderReflector();

    void AddShaderStage(vk::ShaderStageFlagBits stage, const std::vector<std::byte>& spirvBytes);
    vk::Pipeline BuildPipeline();

    NON_COPYABLE(ShaderReflector);
    NON_MOVABLE(ShaderReflector);

private:
    struct ShaderStage
    {
        vk::ShaderStageFlagBits stage;
        std::string entryPoint = "main";
        std::vector<std::byte> spirvBytes; // TODO: Consider keeping this a reference to prevent copy.
        SpvReflectShaderModule reflectModule;
    };

    const VulkanBrain& _brain;
    std::vector<vk::PipelineShaderStageCreateInfo> _pipelineShaderStages;
    std::vector<ShaderStage> _shaderStages;
    vk::PipelineLayout _pipelineLayout;
    std::vector<vk::DescriptorSetLayout> _descriptorSetLayouts;
    std::vector<vk::PushConstantRange> _pushConstantRanges;

    void ReflectShaders();
    void CreateDescriptorLayouts();
    void CreatePipelineLayout();
    vk::Pipeline CreatePipeline();

    vk::ShaderModule CreateShaderModule(const std::vector<std::byte>& spirvBytes);
};