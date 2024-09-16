#pragma once
#include "class_decorations.hpp"
#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.h"
#include "mesh.hpp"

struct VulkanBrain;
struct TextureHandle;

class IBLPipeline
{
public:
    IBLPipeline(const VulkanBrain& brain, const ImageHandle environmentMap);
    ~IBLPipeline();

    void RecordCommands(vk::CommandBuffer commandBuffer);
    const Cubemap& IrradianceMap() const { return _irradianceMap; }
    const Cubemap& PrefilterMap() const { return _prefilterMap; }
    const ImageHandle& BRDFLUTMap() const { return _brdfLUT; }

    NON_MOVABLE(IBLPipeline);
    NON_COPYABLE(IBLPipeline);

private:
    struct PrefilterPushConstant
    {
        uint32_t faceIndex;
        float roughness;
    };

    const VulkanBrain& _brain;
    const ImageHandle _environmentMap;

    vk::PipelineLayout _irradiancePipelineLayout;
    vk::Pipeline _irradiancePipeline;
    vk::PipelineLayout _prefilterPipelineLayout;
    vk::Pipeline _prefilterPipeline;
    vk::PipelineLayout _brdfLUTPipelineLayout;
    vk::Pipeline _brdfLUTPipeline;
    vk::DescriptorSetLayout _descriptorSetLayout;
    vk::DescriptorSet _descriptorSet;

    Cubemap _irradianceMap;
    Cubemap _prefilterMap;
    ImageHandle _brdfLUT;

    std::array<vk::ImageView, 6> _irradianceMapViews;
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