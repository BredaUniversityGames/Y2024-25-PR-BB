#pragma once

#include "common.hpp"
#include "ecs_module.hpp"
#include "imgui_entt_entity_editor.hpp"
#include <memory>

class PerformanceTracker;
class BloomSettings;
class Renderer;
class ImGuiBackend;

class Editor
{
public:
    Editor(ECSModule& ecs, const std::shared_ptr<Renderer>& renderer, const std::shared_ptr<ImGuiBackend>& imguiBackend);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings);

private:
    void DrawMainMenuBar();

    ECSModule& _ecs;
    std::shared_ptr<Renderer> _renderer;
    std::shared_ptr<ImGuiBackend> _imguiBackend;

    entt::entity _selectedEntity = entt::null;

    EnttEditor::EntityEditor<entt::entity> _entityEditor {};

    void DisplaySelectedEntityDetails();
};