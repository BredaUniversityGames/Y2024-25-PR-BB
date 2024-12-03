#include "ui_text.hpp"
#include "fonts.hpp"
#include "pipelines/ui_pipeline.hpp"

void UITextElement::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    UIElement::ChildrenSubmitDrawInfo(drawList);

    float localOffset = 0;

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

            glm::vec2 correctedBearing = (glm::vec2(fontChar.bearing) / static_cast<float>(_font->characterHeight)) * GetScale();
            glm::vec3 translation = glm::vec3(GetAbsoluteLocation() + glm::vec2(localOffset + correctedBearing.x, -correctedBearing.y), 0);
            glm::vec3 localScale = glm::vec3(glm::vec2(fontChar.size) / glm::vec2(_font->characterHeight), 1.0) * glm::vec3(GetScale(), 0);

            info.modelMatrix = (glm::scale(glm::translate(glm::mat4(1), translation), localScale));
            info.textureIndex = _font->fontAtlas.Index();
            info.useRedAsAlpha = true;
            info.color = _color;
            info.uvMin = fontChar.uvMin;
            info.uvMax = fontChar.uvMax;

            drawList.emplace_back(info);

            localOffset += (float(fontChar.advance >> 6) / _font->characterHeight) * GetScale().x; // Convert advance from 1/64th pixels
        }
        else
        {
            localOffset += (static_cast<int>(GetScale().x)) / 4.0;
        }
    }
}
