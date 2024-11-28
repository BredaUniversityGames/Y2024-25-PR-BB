#pragma once
#include "pch.hpp"
#include "ui_element.hpp"

class UIFont;
struct UITextElement : public UIElement
{
    UITextElement(const std::shared_ptr<UIFont>& font)
        : UIElement(0)
        , font(font)
    {
    }
    /**
     * override because this function in UIElement is virtual, text cannot contain children.
     */
    void UpdateAllChildrenAbsoluteLocations() override { }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    const std::shared_ptr<UIFont> font;
    std::string text;
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};
