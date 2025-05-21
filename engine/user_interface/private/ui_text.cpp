#include "ui_text.hpp"
#include "fonts.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void UITextElement::UpdateLocalTextSize()
{
    _horizontalTextSize = 0;

    for (const auto& character : _text)
    {
        // Check if the character exists in the font's character map
        if (!_font->characters.contains(character))
        {
            continue;
        }

        const UIFont::Character& fontChar = _font->characters.at(character);
        _horizontalTextSize += static_cast<float>(fontChar.advance + _font->metrics.charSpacing);
    }
}
void UITextElement::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    UIElement::ChildrenSubmitDrawInfo(drawList);

    glm::vec2 localOffset {};
    float elementScale = GetAbsoluteScale().y / _font->metrics.resolutionY;
    glm::vec2 elementTranslation = GetAbsoluteLocation();

    if (anchorPoint == AnchorPoint::eMiddle || anchorPoint == AnchorPoint::eBottomCenter)
    {
        elementTranslation.x -= (_horizontalTextSize / 2.0f) * elementScale;
    }

    for (const auto& character : _text)
    {
        // Check if the character exists in the font's character map
        if (!_font->characters.contains(character))
        {
            continue;
        }

        UIFont::Character fontChar = _font->characters.at(character);

        QuadDrawInfo info;

        float topBearing = (_font->metrics.ascent - fontChar.bearing.y) * elementScale;
        float leftBearing = fontChar.bearing.x * elementScale;

        glm::vec3 localTranslation = glm::vec3(elementTranslation + localOffset + glm::vec2(leftBearing, topBearing), 0.0f);
        glm::vec3 localScale = glm::vec3(glm::vec2(fontChar.size) * elementScale, 0.0f);

        info.matrix = glm::scale(glm::translate(glm::mat4(1), localTranslation), localScale);
        info.textureIndex = _font->fontAtlas.Index();
        info.useRedAsAlpha = true;
        info.color = _color;

        info.uvMin = fontChar.uvMin;
        info.uvMax = fontChar.uvMax;

        drawList.emplace_back(info);
        localOffset.x += static_cast<float>(fontChar.advance) * elementScale + _font->metrics.charSpacing * elementScale;
    }
}
void UITextElement::SetText(std::string text)
{
    _text = std::move(text);
    UpdateLocalTextSize();
}
