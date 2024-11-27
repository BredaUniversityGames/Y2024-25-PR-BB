#pragma once

#include "quad_draw_info.hpp"
#include <memory>
#include <vector>
class InputManager;
#include "ui_element.hpp"
class UIElement;
class UIPipeline;

// note: right now this is only stored inside the engine, preferably should be linked to the camera.
class Viewport
{
public:
    Viewport(const glm::vec2& extend, const glm::vec2& offset = glm::vec2(0))
        : extend(extend)
        , offset(offset)
    {
    }

    void Update(const InputManager& input) const;

    /**
     * \brief adds all the visible elements to the drawlist of the supplied UIPipeline ,needs to be called before the Renderer renders.
     */
    void Render();

    UIElement& AddElement(std::unique_ptr<UIElement> element);

    /**
     * \brief Base elements present in viewport.
     */
    std::vector<std::unique_ptr<UIElement>> baseElements;

    glm::vec2 extend;
    glm::vec2 offset;

    NO_DISCARD const std::vector<QuadDrawInfo>& GetDrawList() const noexcept
    {
        return _drawList;
    }

private:
    std::vector<QuadDrawInfo> _drawList;
};