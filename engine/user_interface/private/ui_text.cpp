
#include "ui_text.hpp"

#include "ui_pipelines.hpp"
void UITextElement::SubmitDrawInfo(UIPipeline& pipeline) const
{
    auto FontResource = fontResourceManager.Access(font);

    int localOffset = 0;
    for (auto i : text)
    {
        if (i != ' ')
        {
            auto character = FontResource->characters.at(i);

            QuadDrawInfo info;
            info.projection = (glm::scale(glm::translate(glm::mat4(1), glm::vec3(GetAbsouluteLocation() + glm::vec2(localOffset, 0), 0)), glm::vec3(scale * glm::vec2(character.Size), 0))),
            info.textureIndex = FontResource->_fontAtlas.index,
            info.useRedAsAlpha = true;
            info.uvp1 = character.uvp1;
            info.uvp2 = character.uvp2;
            pipeline._drawlist.emplace_back(info);

            localOffset += character.Size.x;
        }
        else
        {
            // temp should be replaced by general font size
            localOffset += scale.x * 30;
        }
     
    }
}
