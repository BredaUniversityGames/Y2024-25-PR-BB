#pragma once

#include "ui_element.hpp"

#include "fonts.hpp"
#include <glm/glm.hpp>
#include <string>
#include <ui_element.hpp>
#include <cereal/types/string.hpp>

struct UIFont;

class UITextElement : public UIElement
{
public:

    UITextElement() : UIElement()
    {
        
    }
    UITextElement(const std::shared_ptr<UIFont>& font, std::string text)
        : _font(font)
    {
        SetLocation(glm::vec2(0));
        SetScale(glm::vec2(0.0f, _font->metrics.resolutionY));
        SetText(std::move(text));
    }

    UITextElement(const std::shared_ptr<UIFont>& font, std::string text, float textSize)
        : _font(font)
    {
        SetLocation(glm::vec2(0));
        SetScale(glm::vec2 { 0.0f, textSize }); 
        SetText(std::move(text));
    }

    UITextElement(const std::shared_ptr<UIFont>& font, std::string text, const glm::vec2& location, float textSize)
        : _font(font)
    {
        SetLocation(location);
        SetScale(glm::vec2 { 0.0f, textSize });
        SetText(std::move(text));
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;

    void SetText(std::string text);
    std::string GetText() const { return _text; }

    void SetFont(std::shared_ptr<UIFont> font) { _font = std::move(font); };
    const UIFont& GetFont() const { return *_font; };
    const std::shared_ptr<UIFont>& GetFontPtr() const { return _font; };

    
private:
    
    friend class cereal::access;
    void UpdateLocalTextSize();
    float _horizontalTextSize = 0;
    std::shared_ptr<UIFont> _font;
    std::string _text;
};

    
template <class Archive>
void save(Archive& ar,const UITextElement& UItext) 
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&UItext)));
    auto text = UItext.GetText();
    ar(CEREAL_NVP(text));
    ar(cereal::make_nvp("font",UItext.GetFontPtr()));
}

template <class Archive>
void load(Archive& ar,UITextElement& UItext) 
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&UItext)));
    std::string text;
    ar(text);
    UItext.SetText(text);
    std::shared_ptr<UIFont> font;
    ar(font);
    UItext.SetFont(font);
}

CEREAL_REGISTER_TYPE(UITextElement);
