#pragma once

#include "mesh.hpp"

#include <memory>

class GraphicsContext;
struct Sampler;

class IBLPipeline
{
public:
    IBLPipeline(const std::shared_ptr<GraphicsContext>& context, ResourceHandle<Image> environmentMap);
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
        uint32_t hdriIndex;
    };

    struct IrradiancePushConstant
    {
        uint32_t index;
        uint32_t hdriIndex;
    };

    std::shared_ptr<GraphicsContext> _context;
    ResourceHandle<Image> _environmentMap;

    vk::PipelineLayout _irradiancePipelineLayout;
    vk::Pipeline _irradiancePipeline;
    vk::PipelineLayout _prefilterPipelineLayout;
    vk::Pipeline _prefilterPipeline;
    vk::PipelineLayout _brdfLUTPipelineLayout;
    vk::Pipeline _brdfLUTPipeline;

    ResourceHandle<Image> _irradianceMap;
    ResourceHandle<Image> _prefilterMap;
    ResourceHandle<Image> _brdfLUT;

    ResourceHandle<Sampler> _sampler;

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