#pragma once

#include "common.hpp"
#include "imgui_entt_entity_editor.hpp"
#include "vulkan/vulkan.hpp"

#include <entt/entity/entity.hpp>

class ECS;
class PerformanceTracker;
class BloomSettings;
class Renderer;
class ImGuiBackend;

struct SceneDescription;

class Editor
{
public:
    Editor(ECS& ecs, Renderer& renderer, const std::shared_ptr<ImGuiBackend>& imguiBackend);

    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene);

private:
    void DrawMainMenuBar();

    ECS& _ecs;
    Renderer& _renderer;
    std::shared_ptr<ImGuiBackend> _imguiBackend;
    vk::UniqueSampler _basicSampler; // Sampler for basic textures/ImGUI images, etc

    entt::entity _selectedEntity = entt::null;

    EnttEditor::EntityEditor<entt::entity> _entityEditor {};

    void DisplaySelectedEntityDetails();
};