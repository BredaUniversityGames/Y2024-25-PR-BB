#pragma once
#include "common.hpp"

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
class RendererModule;

class Editor
{
public:
    Editor(ECS& ecs, RendererModule& renderer);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, SceneDescription& scene, ECS& ecs);

private:
    void DrawMainMenuBar();

    ECS& _ecs;
    RendererModule& _renderer;
    vk::UniqueSampler _basicSampler; // Sampler for basic textures/ImGUI images, etc

    entt::entity _selectedEntity = entt::null;

    void DisplaySelectedEntityDetails(ECS& ecs);
};