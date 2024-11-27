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
     * recursively adds all the draw data for the ui to the drawList argument.
     * This drawList gets cleared when the uiPipeline records it's commands and thus this function needs to be called before the commandLists are submitted.
     * @param drawList Reference to the vector holding the QuadDrawInfo, which is most likely located in the uiPipeline.
     */
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const;

    UIElement& AddElement(std::unique_ptr<UIElement> element);

    /**
     * \brief Base elements present in viewport.
     */
    std::vector<std::unique_ptr<UIElement>> baseElements;

    glm::vec2 extend;
    glm::vec2 offset;

private:
};