#pragma once
#include "common.hpp"

class Renderer;

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
    Editor(ECS& ecs,Renderer& renderer);

    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene);

private:

    void DrawMainMenuBar();

    ECS& _ecs;
    Renderer& _renderer;
    vk::UniqueSampler _basicSampler; // Sampler for basic textures/ImGUI images, etc


    entt::entity _selectedEntity = entt::null;

    void DisplaySelectedEntityDetails(ECS& ecs);
};