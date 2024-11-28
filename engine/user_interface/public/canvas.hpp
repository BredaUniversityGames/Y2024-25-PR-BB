#pragma once
#include "ui_element.hpp"

struct QuadDrawInfo;

struct Canvas : public UIElement
{
public:
    Canvas(const glm::vec2& size)
        : UIElement(std::numeric_limits<uint16_t>::max())
    {
        SetScale(size);
    }

    void UpdateAllChildrenAbsoluteLocations() override;
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;
};
