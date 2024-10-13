#pragma once
#include "common.hpp"

class VulkanBrain;
class PerformanceTracker;
class BloomSettings;
struct SceneDescription;
class GBuffers;
class Editor
{
public:
    Editor(const VulkanBrain& brain, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages, GBuffers& gBuffers);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene);

private:
    const VulkanBrain& _brain;
    vk::UniqueSampler _basicSampler; // Sampler for basic textures/ImGUI images, etc
    GBuffers& _gBuffers;
};