#pragma once

#include "common.hpp"
#include "imgui_entt_entity_editor.hpp"

#include <entt/entity/entity.hpp>
#include <memory>

class ECS;
class PerformanceTracker;
class BloomSettings;
class Renderer;
class ImGuiBackend;
struct SceneDescription;

class Editor
{
public:
    Editor(const std::shared_ptr<ECS>& ecs, const std::shared_ptr<Renderer>& renderer, const std::shared_ptr<ImGuiBackend>& imguiBackend);

    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene);

private:
    void DrawMainMenuBar();

    std::shared_ptr<ECS> _ecs;
    std::shared_ptr<Renderer> _renderer;
    std::shared_ptr<ImGuiBackend> _imguiBackend;

    entt::entity _selectedEntity = entt::null;

    EnttEditor::EntityEditor<entt::entity> _entityEditor {};

    void DisplaySelectedEntityDetails();
};