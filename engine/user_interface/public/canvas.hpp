#pragma once
#include "ui_element.hpp"

struct QuadDrawInfo;

class Canvas : public UIElement
{
public:
    Canvas(const glm::vec2& size, UINavigationMappings::ElementMap elementMap = {})
        : UIElement(std::move(elementMap))
    {
        SetScale(size);
    }
    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;
};
