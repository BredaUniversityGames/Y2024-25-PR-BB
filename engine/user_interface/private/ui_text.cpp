
#include "ui_text.hpp"
#include "pipelines/ui_pipeline.hpp"

#include <fonts.hpp>

void UITextElement::SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const
{

    float localOffset = 0;

    for (const auto& i : text)
    {
        if (i != ' ')
        {
            // Check if the character exists in the font's character map
            if (!_font->characters.contains(i))
            {
                continue;
            }

            const auto& character = _font->characters.at(i);

            QuadDrawInfo info;

            glm::vec3 translation = glm::vec3(GetAbsouluteLocation() + glm::vec2(localOffset + character.Bearing.x, -character.Bearing.y), 0);
            glm::vec3 scale = glm::vec3(character.Size, 1.0) * glm::vec3(GetScale(), 0);
            info.modelMatrix = (glm::scale(glm::translate(glm::mat4(1), translation), scale));
            info.textureIndex = _font->_fontAtlas.Index();
            info.useRedAsAlpha = true;
            info.uvp1 = character.uvp1;
            info.uvp2 = character.uvp2;

            drawList.emplace_back(info);

            localOffset += (character.Advance >> 6) * GetScale().x; // Convert advance from 1/64th pixels
        }
        else
        {
            localOffset += (static_cast<int>(GetScale().x) * _font->characterHeight) / 4.0;
        }
    }
}
