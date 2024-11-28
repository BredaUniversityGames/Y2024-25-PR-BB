#pragma once

#include "quad_draw_info.hpp"
#include "ui_element.hpp"
#include <memory>
#include <vector>

class InputManager;
class UIElement;

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
     * adds all the draw data for the ui to the drawList argument, fynction calls SubmitDrawInfo on all the present uiElements in a hierarchical manner.
     * This drawList gets cleared when the uiPipeline records it's commands and thus this function needs to be called before the commandLists are submitted.
     * @param drawList Reference to the vector holding the QuadDrawInfo, which is most likely located in the uiPipeline.
     */
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const;

    UIElement& AddElement(std::unique_ptr<UIElement> element);

    NO_DISCARD std::vector<std::unique_ptr<UIElement>>& GetElements() { return _baseElements; }
    NO_DISCARD const std::vector<std::unique_ptr<UIElement>>& GetElements() const { return _baseElements; }

    glm::vec2 extend;
    glm::vec2 offset;

private:
    /**
     * \brief Base elements present in viewport.
     */
    std::vector<std::unique_ptr<UIElement>> _baseElements;
};