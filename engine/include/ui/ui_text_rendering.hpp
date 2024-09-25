//
// Created by luuk on 25-9-2024.
//

#pragma once
#include "pch.hpp"
#include "UserInterfaceSystem.h"

struct TextDrawInfo
{
    glm::vec2 position;
    uint16_t m_FontSize;
    uint16_t m_FontSpacing;
    std::string_view text;
};

struct UITextElement : public UIElement
{
    void UpdateChildAbsoluteLocations() override { }
    void SubmitDrawInfo(UserInterfaceContext&) override;
    std::string m_Text;
};

class UITextRenderSystem : public UIRenderSystem<TextDrawInfo>
{
public:
    UITextRenderSystem(const UIPipeLine& pl)
        : UIRenderSystem<TextDrawInfo>(pl)
    {
    }
    void Render(const vk::CommandBuffer& commandBuffer, const glm::mat4& projection_matrix) override;

    ~UITextRenderSystem() = default;
};
