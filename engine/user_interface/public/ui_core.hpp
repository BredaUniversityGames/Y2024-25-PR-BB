#pragma once
#include "../public/ui_element.hpp"
#include "typeindex"
#include <queue>
class UIPipeline;

/**
 * holds free floating elements. elements can be anchored to one of the 4 corners of the canvas. anchors help preserve
 * the layout across different resolutions.
 */
struct Canvas : public UIElement
{
public:
    Canvas(const glm::vec2& size)
        : UIElement(std::numeric_limits<uint16_t>::max())
    {
        this->scale = size;
    }
    void UpdateChildAbsoluteLocations() override;
    void SubmitDrawInfo(UIPipeline& pipeline) const override;
};
