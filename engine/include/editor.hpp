#pragma once
#include "common.hpp"

#include <entt/entity/entity.hpp>

class ECS;
class PhysicsModule;
class VulkanBrain;
class PerformanceTracker;
class BloomSettings;
struct SceneDescription;
class GBuffers;
class ECS;
class Editor
{
public:

    Editor(const VulkanBrain& brain, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages, GBuffers& gBuffers,ECS&);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene, ECS& ecs);

private:
    ECS& _ecs;
    const VulkanBrain& _brain;
    vk::UniqueSampler _basicSampler; // Sampler for basic textures/ImGUI images, etc
    GBuffers& _gBuffers;

    entt::entity _selectedEntity = entt::null;

    void DisplaySelectedEntityDetails(ECS& ecs);
};