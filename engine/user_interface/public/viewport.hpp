#pragma once

#include <memory>
#include <vector>

class InputManager;
class UIElement;
class UIPipeline;

// note: right now this is only stored inside the engine, preferably should be linked to the camera.
class Viewport
{
public:
    void Update(const InputManager& input) const;

    /**
     * \brief adds all the visible elements to the drawlist of the supplied UIPipeline ,needs to be called before the Renderer renders.
     */
    void Render(UIPipeline& pipeline) const;

    /**
     * \brief Base elements present in viewport.
     */
    std::vector<std::unique_ptr<UIElement>> baseElements;

    void AddElement(std::unique_ptr<UIElement> element);
};