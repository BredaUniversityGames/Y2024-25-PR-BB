
#include "ui_text.hpp"

#include "ui_pipelines.hpp"
void UITextElement::SubmitDrawInfo(UIPipeline& pipeline) const
{
    const auto* fontResource = fontResourceManager.Access(font);

    if (fontResource == nullptr)
    {
        return;
    }

    int localOffset = 0;

    for (const auto& i : text)
    {
        if (i != ' ')
        {
            // Check if the character exists in the font's character map
            if (fontResource->characters.find(i) == fontResource->characters.end())
            {
                continue;
            }

            const auto& character = fontResource->characters.at(i);

            QuadDrawInfo info;
            info.projection = (glm::scale(glm::translate(glm::mat4(1), glm::vec3(GetAbsouluteLocation() + glm::vec2(localOffset + character.Bearing.x, -character.Bearing.y), 0)), glm::vec3(character.Size, 1.0) * glm::vec3(scale, 0)));
            info.textureIndex = fontResource->_fontAtlas.index;
            info.useRedAsAlpha = true;
            info.uvp1 = character.uvp1;
            info.uvp2 = character.uvp2;

            pipeline._drawlist.emplace_back(info);

            localOffset += (character.Advance >> 6) * scale.x; // Convert advance from 1/64th pixels
        }
        else
        {
            localOffset += (static_cast<int>(scale.x) * fontResource->characterHeight) / 4;
        }
    }
}
