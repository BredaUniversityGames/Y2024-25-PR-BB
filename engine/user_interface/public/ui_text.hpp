#pragma once
#include "pch.hpp"
#include "ui_element.hpp"

class UIFont;
class UITextElement : public UIElement
{
    UITextElement(const std::shared_ptr<UIFont>& font)
        : font(font)
    {
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    const std::shared_ptr<UIFont> font;
    std::string text;
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};
