//
// Created by luuk on 16-9-2024.
//

#pragma once
#include "typeindex"
#include <queue>
#include "../public/ui_element.hpp"
class UIPipeline;

/**
 * holds free floating elements. elements can be anchored to one of the 4 corners of the canvas. anchors help preserve
 * the layout across different resolutions.
 */
struct Canvas : public UIElement
{
public:
    Canvas()
        : UIElement(std::numeric_limits<uint16_t>::max())
    {
    }
    void UpdateChildAbsoluteLocations() override;
    void SubmitDrawInfo(UIPipeline& pipeline) const override;
};
