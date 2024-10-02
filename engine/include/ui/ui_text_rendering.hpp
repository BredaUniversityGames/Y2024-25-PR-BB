//
// Created by luuk on 25-9-2024.
//

#pragma once
#include "pch.hpp"
#include "UserInterfaceSystem.hpp"

struct TextDrawInfo
{
    glm::vec2 position;
    uint16_t m_FontSize;
    uint16_t m_FontSpacing;
    std::string_view text;
};

struct UITextElement : public UIElement
{
    UITextElement()
        : UIElement(0)
    {
    }
    void UpdateChildAbsoluteLocations() override { }
    void SubmitDrawInfo(UserInterfaceRenderContext&) const override;
    std::string m_Text;
};

class UITextRenderSystem : public UIRenderSystem<TextDrawInfo>
{
public:
    UITextRenderSystem(std::shared_ptr<UIPipeLine>& pl)
        : UIRenderSystem<TextDrawInfo>(pl)
    {
    }

    void Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix, const VulkanBrain&) override;

    ~UITextRenderSystem() override = default;
};
