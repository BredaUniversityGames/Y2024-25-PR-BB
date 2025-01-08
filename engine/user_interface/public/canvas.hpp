#pragma once
#include "ui_element.hpp"

struct QuadDrawInfo;

class Canvas : public UIElement
{
public:
    Canvas(const glm::vec2& size)
        : UIElement()
    {
        SetScale(size);
    }
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;
};
