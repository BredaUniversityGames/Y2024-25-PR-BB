#pragma once
#include "vulkan/vulkan.hpp"
#include "mesh.hpp"

class VulkanBrain;
struct TextureHandle;

class IBLPipeline
{
public:
    IBLPipeline(const VulkanBrain& brain, ResourceHandle<Image> environmentMap);
    ~IBLPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer);
    ResourceHandle<Image> IrradianceMap() const { return _irradianceMap; }
    ResourceHandle<Image> PrefilterMap() const { return _prefilterMap; }
    ResourceHandle<Image> BRDFLUTMap() const { return _brdfLUT; }

    NON_MOVABLE(IBLPipeline);
    NON_COPYABLE(IBLPipeline);

private:
    struct PrefilterPushConstant
    {
        uint32_t faceIndex;
        float roughness;
    };

    const VulkanBrain& _brain;
    const ResourceHandle<Image> _environmentMap;

    vk::PipelineLayout _irradiancePipelineLayout;
    vk::Pipeline _irradiancePipeline;
    vk::PipelineLayout _prefilterPipelineLayout;
    vk::Pipeline _prefilterPipeline;
    vk::PipelineLayout _brdfLUTPipelineLayout;
    vk::Pipeline _brdfLUTPipeline;
    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;

    ResourceHandle<Image> _irradianceMap;
    ResourceHandle<Image> _prefilterMap;
    ResourceHandle<Image> _brdfLUT;

    vk::Sampler _sampler;

    std::vector<std::array<vk::ImageView, 6>> _prefilterMapViews;

    void CreateIrradiancePipeline();
    void CreatePrefilterPipeline();
    void CreateBRDFLUTPipeline();
    void CreateDescriptorSetLayout();
    void CreateDescriptorSet();
    void CreateIrradianceCubemap();
    void CreatePrefilterCubemap();
    void CreateBRDFLUT();
};