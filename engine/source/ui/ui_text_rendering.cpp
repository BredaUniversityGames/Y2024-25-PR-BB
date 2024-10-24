//
// Created by luuk on 25-9-2024.
//

#include "ui/ui_text.hpp"
#include "../../user_interface/public/ui_pipelines.hpp"
#include "ui/fonts.hpp"

void UITextRenderSystem::Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix, const VulkanBrain& brain)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipeLine->m_pipeline);

    auto size = renderQueue.size();
    for (int i = 0; i < size; i++)
    {
        const auto& render_info = renderQueue.front();

        auto matrix = glm::mat4(1.f);
        matrix = glm::translate(matrix, glm::vec3(render_info.position.x, render_info.position.y, 0));

        for (const auto c : render_info.text)
        {
            if (c == ' ')
            {
                matrix = glm::translate(matrix, glm::vec3(render_info.m_FontSize + render_info.m_FontSpacing, 0, 0));
                continue;
            }
            if (Fonts::Characters.find(c) == Fonts::Characters.end())
            {
                continue;
            }
            Fonts::Character ch = Fonts::Characters[c];

            GenericUIPushConstants constants {
                .m_ProjectionMatrix = projection_matrix * (glm::scale(matrix, glm::vec3(ch.Size.x, ch.Size.y, 1))),
                .m_TextureIndex = ch.image.index
            };

            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipeLine->m_pipelineLayout, 0, 1,
                &brain.bindlessSet, 0, nullptr);
            commandBuffer.pushConstants(m_PipeLine->m_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(GenericUIPushConstants), &constants);

            commandBuffer.draw(6, 1, 0, 0);

            matrix = glm::translate(matrix, glm::vec3(ch.Size.x + render_info.m_FontSpacing, 0, 0));
        }

        renderQueue.pop();
    }
};

void UITextElement::SubmitDrawInfo(UserInterfaceRenderer& user_interface_context) const
{
    user_interface_context.GetRenderingSystem<UITextRenderSystem>()->renderQueue.push({ m_AbsoluteLocation, 10, 2, m_Text });
    for (auto& i : GetChildren())
    {
        i->SubmitDrawInfo(user_interface_context);
    }
}
