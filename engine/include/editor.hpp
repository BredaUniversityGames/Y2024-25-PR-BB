#pragma once
#include "class_decorations.hpp"

class VulkanBrain;
class Application;
class PerformanceTracker;
class BloomSettings;
struct SceneDescription;

class Editor
{
public:
    Editor(const VulkanBrain& brain, Application& application, vk::Format swapchainFormat, vk::Format depthFormat, uint32_t swapchainImages);
    ~Editor();

    NON_MOVABLE(Editor);
    NON_COPYABLE(Editor);

    void Draw(PerformanceTracker& performanceTracker, BloomSettings& bloomSettings, SceneDescription& scene);

private:
    const VulkanBrain& _brain;
    Application& _application;
};