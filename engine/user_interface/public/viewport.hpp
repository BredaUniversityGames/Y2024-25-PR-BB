#pragma once
#include "quad_draw_info.hpp"
#include "ui_element.hpp"
#include <memory>
#include <vector>

class InputDeviceManager;
class UIElement;

class Viewport
{
public:
    Viewport(const glm::uvec2& extend, const glm::uvec2& offset = glm::uvec2(0))
        : _extend(extend)
        , _offset(offset)
    {
    }
    void Update(const ActionManager& input) const;
    /**
     * adds all the draw data for the ui to the drawList argument, fynction calls SubmitDrawInfo on all the present uiElements in a hierarchical manner.
     * This drawList gets cleared when the uiPipeline records it's commands and thus this function needs to be called before the commandLists are submitted.
     * @param drawList Reference to the vector holding the QuadDrawInfo, which is most likely located in the uiPipeline.
     */
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const;

    UIElement& AddElement(std::unique_ptr<UIElement> element);

    NO_DISCARD std::vector<std::unique_ptr<UIElement>>& GetElements() { return _baseElements; }
    NO_DISCARD const std::vector<std::unique_ptr<UIElement>>& GetElements() const { return _baseElements; }

    const glm::uvec2& GetExtend() { return _extend; }
    const glm::uvec2& GetOffset() { return _offset; }

    void SetExtend(const glm::uvec2 extend) { _extend = extend; }
    void SetOffset(const glm::uvec2 offset) { _offset = offset; }

private:
    glm::uvec2 _extend;
    glm::uvec2 _offset;

    /**
     * \brief Base elements present in viewport.
     */
    std::vector<std::unique_ptr<UIElement>> _baseElements;
};