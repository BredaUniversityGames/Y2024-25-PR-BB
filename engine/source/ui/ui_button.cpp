//
// Created by luuk on 30-9-2024.
//

#include "ui/ui_button.hpp"
#include "input_manager.hpp"
#include "pipelines/ui_pipelines.hpp"

void UIButton::Update(const InputManager& input)
{
    glm::ivec2 mousePos;
    input.GetMousePosition(mousePos.x, mousePos.y);

    // mouse inside boundary
    if (mousePos.x > static_cast<int>(m_AbsoluteLocation.x)
        && mousePos.x < static_cast<int>(m_AbsoluteLocation.x + m_Scale.x)
        && mousePos.y > static_cast<int>(m_AbsoluteLocation.y)
        && mousePos.y < static_cast<int>(m_AbsoluteLocation.y + m_Scale.y))
    {
        switch (m_State)
        {
        case ButtonState::NORMAL:

            m_State = ButtonState::HOVERED;
            m_OnBeginHoverCallBack();
            [[fallthrough]];

        case ButtonState::HOVERED:

            if (input.IsMouseButtonPressed(InputManager::MouseButton::Left))
            {
                m_State = ButtonState::PRESSED;
                m_OnMouseDownCallBack();
            }
            break;

        case ButtonState::PRESSED:
            if (input.IsMouseButtonReleased(InputManager::MouseButton::Left))
            {
                m_State = ButtonState::NORMAL;
            }
            break;
        }
    }

    else
        m_State = ButtonState::NORMAL;
}

void UIButton::SubmitDrawInfo(UserInterfaceRenderer& user_interface_context) const
{

    ResourceHandle<Image> image;
    switch (m_State)
    {
    case ButtonState::NORMAL:
        image = m_Style.m_NormalImage;
        break;

    case ButtonState::HOVERED:
        image = m_Style.m_HoveredImage;
        break;

    case ButtonState::PRESSED:
        image = m_Style.m_PressedImage;
        break;
    }

    user_interface_context.GetRenderingSystem<UIButtonRenderSystem>()->renderQueue.push(ButtonDrawInfo {
        .position = m_AbsoluteLocation,
        .Scale = this->m_Scale,
        .Image = image });

    for (auto& i : GetChildren())
    {
        i->SubmitDrawInfo(user_interface_context);
    }
}
void UIButton::UpdateChildAbsoluteLocations()
{
    for (auto& i : GetChildren())
        i->UpdateAbsoluteLocation(m_AbsoluteLocation);
}

void UIButtonRenderSystem::Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix, const VulkanBrain& brain)
{
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_PipeLine->m_pipeline);

    auto size = renderQueue.size();
    for (int i = 0; i < size; i++)
    {
        const auto& info = renderQueue.front();
        glm::mat4 matrix = glm::mat4(1);
        matrix = glm::scale(glm::translate(matrix, glm::vec3(info.position, 0)), glm::vec3(info.Scale, 0));
        glm::mat4 s = glm::ortho(0.0f, 2640.0f, 0.0f, 1600.0f) * matrix;
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipeLine->m_pipelineLayout, 0, 1, &brain.bindlessSet, 0, nullptr);

        // nescesery beccause index is a bit-field.
        auto imageIndex = info.Image.index;

        vkCmdPushConstants(commandBuffer, m_PipeLine->m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
            &s);
        vkCmdPushConstants(commandBuffer, m_PipeLine->m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(uint32_t),
            &imageIndex);
        commandBuffer.draw(6, 1, 0, 0);

        renderQueue.pop();
    }
}
