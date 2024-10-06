//
// Created by luuk on 6-10-2024.
//
#pragma once
#include "pch.hpp"

class VulkanBrain;
class UIPipeLine
{
public:
    void CreatePipeLine(std::string_view vertshader, std::string_view fragshader);
    explicit UIPipeLine(const VulkanBrain& brain)
        : m_brain(brain) {};

    NON_COPYABLE(UIPipeLine);
    NON_MOVABLE(UIPipeLine);

    ~UIPipeLine();

    VkPipeline m_pipeline;
    vk::PipelineLayout m_pipelineLayout;
    vk::DescriptorSet m_descriptorSet {};

    const VulkanBrain& m_brain;
    vk::UniqueSampler m_sampler;
};
