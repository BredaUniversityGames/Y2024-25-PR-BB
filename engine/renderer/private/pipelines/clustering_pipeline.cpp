#include "pipelines/clustering_pipeline.hpp"

#include "gpu_scene.hpp"

#include "graphics_context.hpp"
#include "pipeline_builder.hpp"
#include "shaders/shader_loader.hpp"
#include "vulkan_context.hpp"

ClusteringPipeline::ClusteringPipeline(const std::shared_ptr<GraphicsContext>& context, const GBuffers& gBuffers)
    : _pushConstants()
    , _context(context)
    , _gBuffers(gBuffers)
{
    CreatePipeline();
}

ClusteringPipeline::~ClusteringPipeline()
{
    _context->VulkanContext()->Device().destroy(_pipeline);
    _context->VulkanContext()->Device().destroy(_pipelineLayout);
}

void ClusteringPipeline::RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, const RenderSceneDescription& scene)
{

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);

    commandBuffer.pushConstants<PushConstants>(_pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, _pushConstants);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 0, { _context->BindlessSet() }, {});
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelineLayout, 1, { scene.gpuScene->MainCamera().DescriptorSet(currentFrame) }, {});
}

void ClusteringPipeline::CreatePipeline()
{
    std::vector<std::byte> compShader = shader::ReadFile("shaders/clustering.comp.spv");

    PipelineBuilder reflector { _context };
    reflector.AddShaderStage(vk::ShaderStageFlagBits::eCompute, compShader);

    reflector.BuildPipeline(_pipeline, _pipelineLayout);
}
