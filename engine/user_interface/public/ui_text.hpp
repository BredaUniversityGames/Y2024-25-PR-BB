
#pragma once
#include "pch.hpp"
#include "ui_element.hpp"

class UIPipeline;
struct UITextElement : public UIElement
{
    UITextElement(const ResourceManager<Font>& fontResourceManager)
        : UIElement(0)
        , fontResourceManager(fontResourceManager)
    {
    }
    void UpdateChildAbsoluteLocations() override { }
    void SubmitDrawInfo(UIPipeline& pipeline) const override;

    const ResourceManager<Font>& fontResourceManager;
    ResourceHandle<Font> font;
    std::string text;
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};
