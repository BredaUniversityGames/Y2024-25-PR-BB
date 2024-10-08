#pragma once
#include "class_decorations.hpp"

#include <entity/entity.hpp>

class ECS;
class VulkanBrain;
class Application;
class PerformanceTracker;
class BloomSettings;
struct SceneDescription;
class GBuffers;
class Editor
{
public:
    Editor(const VulkanBrain& brain, Application& application, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages, GBuffers& gBuffers);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene, ECS& ecs);

private:
    const VulkanBrain& _brain;
    Application& _application;
    vk::UniqueSampler _basicSampler; // Sampler for basic textures/ImGUI images, etc
    GBuffers& _gBuffers;

    entt::entity _selectedEntity = entt::null;

    void DisplaySelectedEntityDetails(ECS& ecs);
};