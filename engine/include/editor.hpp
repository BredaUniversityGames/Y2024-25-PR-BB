#pragma once
#include "common.hpp"
#include "imgui_entt_entity_editor.hpp"

#include "vulkan/vulkan.hpp"

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
    Editor(const VulkanBrain& brain, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages, GBuffers& gBuffers, ECS& ecs);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene, ECS& ecs);

private:
    void DrawMainMenuBar();

    ECS& _ecs;
    const VulkanBrain& _brain;
    vk::UniqueSampler _basicSampler; // Sampler for basic textures/ImGUI images, etc
    GBuffers& _gBuffers;

    entt::entity _selectedEntity = entt::null;

    MM::EntityEditor<entt::entity> _entityEditor {};

    void DisplaySelectedEntityDetails();
};