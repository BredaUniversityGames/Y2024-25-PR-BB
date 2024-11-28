//
// Created by luuk on 16-9-2024.
//

#pragma once
#include "../public/ui_element.hpp"
#include "typeindex"
#include <queue>

struct QuadDrawInfo;
class UIPipeline;

struct Canvas : public UIElement
{
public:
    Canvas(const glm::vec2& size)
        : UIElement(std::numeric_limits<uint16_t>::max())
    {
        SetScale(size);
    }

    void UpdateAllChildrenAbsoluteLocations() override;
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawliust) const override;
};
