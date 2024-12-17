#pragma once

#include "ui_element.hpp"

#include <glm/glm.hpp>
#include <string>

struct UIFont;

class UITextElement : public UIElement
{
public:

    UITextElement(const std::shared_ptr<UIFont>& font, std::string text,UINavigationMappings::ElementMap elementMap ={})
        : UIElement(elementMap), _font(font), _text(std::move(text))
    {
        SetLocation(glm::vec2(0));
        SetScale(glm::vec2(0, 50));
        SetText(std::move(text));
    }

    UITextElement(const std::shared_ptr<UIFont>& font, std::string text, const glm::vec2& location, float textSize)
        : _font(font)
    {
        SetLocation(location);
        SetScale(glm::vec2(0, textSize));
        SetText(std::move(text));
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void SetColor(glm::vec4 color) { _color = std::move(color); };
    const glm::vec4& GetColor() const { return _color; }

    void SetText(std::string text);
    std::string GetText() const { return _text; }

    void SetFont(std::shared_ptr<UIFont> font) { _font = std::move(font); };
    const UIFont& GetFont() const { return *_font; };

private:
    void UpdateLocalTextSize();
    float _horizontalTextSize = 0;
    std::shared_ptr<UIFont> _font;
    std::string _text;
    glm::vec4 _color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};
