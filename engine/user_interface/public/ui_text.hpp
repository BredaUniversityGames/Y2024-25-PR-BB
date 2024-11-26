/*#pragma once
#include "ui_element.hpp"

class UIPipeline;
struct UITextElement : public UIElement
{
    UITextElement()
        : UIElement(0)
    {
    }
    void UpdateChildAbsoluteLocations() override { }
    void SubmitDrawInfo(UIPipeline& pipeline) const override;

    // const ResourceManager<Font>& fontResourceManager;
    // ResourceHandle<Font> font;
    std::string text;
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
};*/
