//
// Created by luuk on 25-9-2024.
//

#include "ui/ui_text_rendering.hpp"
#include "pipelines/ui_pipelines.hpp"
#include "ui/fonts.hpp"

void UITextRenderSystem::Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix, const VulkanBrain& brain)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipeLine->m_pipeline);

    auto size = renderQueue.size();
    for (int i = 0; i < size; i++)
    {
        const auto& render_info = renderQueue.front();

        auto matrix = glm::mat4(1.f);
        const glm::mat4 s = glm::ortho(0.0f, 2560.0f, 0.0f, 1600.0f) * matrix;
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

            auto finalmatrix = s * (glm::scale(matrix, glm::vec3(ch.Size.x, ch.Size.y, 1)));

            auto imageIndex = ch.image.index;

            vkCmdPushConstants(commandBuffer, m_PipeLine->m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                &finalmatrix);
            vkCmdPushConstants(commandBuffer, m_PipeLine->m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(uint32_t),
                &imageIndex);
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipeLine->m_pipelineLayout, 0, 1,
                &brain.bindlessSet, 0, nullptr);

            commandBuffer.draw(6, 1, 0, 0);

            matrix = glm::translate(matrix, glm::vec3(ch.Size.x + render_info.m_FontSpacing, 0, 0));
        }

        renderQueue.pop();
    }
};

void UITextElement::SubmitDrawInfo(UserInterfaceRenderer& user_interface_context) const
{
    user_interface_context.GetRenderingSystem<UITextRenderSystem>()->renderQueue.push({ AbsoluteLocation, 10, 2, m_Text });
    for (auto& i : GetChildren())
    {
        i->SubmitDrawInfo(user_interface_context);
    }
}
