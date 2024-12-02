#include "ui_text.hpp"
#include "fonts.hpp"
#include "pipelines/ui_pipeline.hpp"

void UITextElement::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{
    float localOffset = 0;

    for (const auto& i : text)
    {
        if (i != ' ')
        {
            // Check if the character exists in the font's character map
            if (!font->characters.contains(i))
            {
                continue;
            }

            const UIFont::Character& character = font->characters.at(i);

            QuadDrawInfo info;

            glm::vec2 correctedBearing = (glm::vec2(character.bearing) / static_cast<float>(font->characterHeight)) * GetScale();
            glm::vec3 translation = glm::vec3(GetAbsouluteLocation() + glm::vec2(localOffset + correctedBearing.x, -correctedBearing.y), 0);
            glm::vec3 localScale = glm::vec3(glm::vec2(character.size) / glm::vec2(font->characterHeight), 1.0) * glm::vec3(GetScale(), 0);

            info.modelMatrix = (glm::scale(glm::translate(glm::mat4(1), translation), localScale));
            info.textureIndex = font->fontAtlas.Index();
            info.useRedAsAlpha = true;
            info.color = color;
            info.uvMin = character.uvMin;
            info.uvMax = character.uvMax;

            drawList.emplace_back(info);

            localOffset += (float(character.advance >> 6) / font->characterHeight) * GetScale().x; // Convert advance from 1/64th pixels
        }
        else
        {
            localOffset += (static_cast<int>(GetScale().x)) / 4.0;
        }
    }
}
