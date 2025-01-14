#pragma once

#include "frame_graph.hpp"

class CameraBatch;
struct Sampler;

class BuildHzbPass final : public FrameGraphRenderPass
{
public:
    BuildHzbPass(const std::shared_ptr<GraphicsContext>& context, CameraBatch& cameraBatch);
    ~BuildHzbPass() final;
    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene) final;

private:
    std::shared_ptr<GraphicsContext> _context;
    CameraBatch& _cameraBatch;

    vk::PipelineLayout _buildHzbPipelineLayout;
    vk::Pipeline _buildHzbPipeline;
    vk::DescriptorSetLayout _hzbImageDSL;
    vk::DescriptorUpdateTemplate _hzbUpdateTemplate;

    ResourceHandle<Sampler> _hzbSampler;

    void CreateSampler();
    void CreateDSL();
    void CreatPipeline();
    void CreateUpdateTemplate();
};
