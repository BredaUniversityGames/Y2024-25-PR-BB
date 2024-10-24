//
// Created by luuk on 6-10-2024.
//
#pragma once
#include "../../include/pch.hpp"

class VulkanBrain;
class UIPipeline
{
public:
    void CreatePipeLine(std::string_view vertshader, std::string_view fragshader);
    explicit UIPipeline(const VulkanBrain& brain)
        : m_brain(brain) {};

    NON_COPYABLE(UIPipeline);
    NON_MOVABLE(UIPipeline);

    void RecordCommands(vk::CommandBuffer commandBuffer, uint32_t currentFrame, uint32_t swapChainIndex);

    ~UIPipeline();

    VkPipeline m_pipeline;
    vk::PipelineLayout m_pipelineLayout;
    vk::DescriptorSet m_descriptorSet {};

    const VulkanBrain& m_brain;
    vk::UniqueSampler m_sampler;
};
