#include "ui_text.hpp"
#include "fonts.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void UITextElement::UpdateLocalTextSize()
{

    _horizontalTextSize = 0;
    for (const auto& character : _text)
    {
        if (character != ' ')
        {
            // Check if the character exists in the font's character map
            if (!_font->characters.contains(character))
            {
                continue;
            }

            const UIFont::Character& fontChar = _font->characters.at(character);

            _horizontalTextSize += (float(fontChar.advance >> 6) / _font->characterHeight); // Convert advance from 1/64th pixels
        }
        else
        {
            _horizontalTextSize += 1.0f / 4.0;
        }
    }
}

void UITextElement::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    UIElement::ChildrenSubmitDrawInfo(drawList);

    float localOffset = 0;

    glm::vec2 elementTranslation = GetAbsoluteLocation();
    if (anchorPoint == AnchorPoint::eMiddle)
    {

        elementTranslation.x -= (_horizontalTextSize / 2.0f) * GetAbsoluteScale().y;
        elementTranslation.y += GetAbsoluteScale().y / 2.0;
    }

    for (const auto& character : _text)
    {
        if (character != ' ')
        {
            // Check if the character exists in the font's character map
            if (!_font->characters.contains(character))
            {
                continue;
            }

            const UIFont::Character& fontChar = _font->characters.at(character);

            QuadDrawInfo info;

            glm::vec2 correctedBearing = (glm::vec2(fontChar.bearing) / static_cast<float>(_font->characterHeight)) * (GetAbsoluteScale().y / 2.0f);
            glm::vec3 localTranslation = glm::vec3(elementTranslation + glm::vec2(localOffset + correctedBearing.x, -correctedBearing.y), 0);
            glm::vec3 localScale = glm::vec3(glm::vec2(fontChar.size) / glm::vec2(_font->characterHeight), 1.0) * glm::vec3(GetAbsoluteScale().y, GetAbsoluteScale().y, 0);

            info.matrix = glm::scale(glm::translate(glm::mat4(1), localTranslation), localScale);
            info.textureIndex = _font->fontAtlas.Index();
            info.useRedAsAlpha = true;
            info.color = _color;
            info.uvMin = fontChar.uvMin;
            info.uvMax = fontChar.uvMax;

            drawList.emplace_back(info);

            localOffset += (float(fontChar.advance >> 6) / _font->characterHeight) * GetAbsoluteScale().y; // Convert advance from 1/64th pixels
        }
        else
        {
            localOffset += (static_cast<int>(GetAbsoluteScale().y)) / 4.0;
        }
    }
}
void UITextElement::SetText(std::string text)
{
    _text = std::move(text);
    UpdateLocalTextSize();
}
