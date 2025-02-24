#pragma once

#include "common.hpp"
#include "ecs_module.hpp"
#include "imgui_entt_entity_editor.hpp"

class PerformanceTracker;
class BloomSettings;
class Renderer;
class ImGuiBackend;

class Editor
{
public:
    Editor(ECSModule& ecs);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void DrawHierarchy();

    void DrawEntityEditor();

private:
    ECSModule& _ecs;

    entt::entity _selectedEntity = entt::null;
    EnttEditor::EntityEditor<entt::entity> _entityEditor {};

    void DisplaySelectedEntityDetails();
};