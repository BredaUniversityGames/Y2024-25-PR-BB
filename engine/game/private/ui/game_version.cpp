#include "fonts.hpp"
#include "graphics_context.hpp"
#include "ui/ui_menus.hpp"
#include "ui_text.hpp"

std::shared_ptr<GameVersionVisualization> GameVersionVisualization::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font, const std::string& text)
{
    auto visualization = std::make_shared<GameVersionVisualization>(screenResolution);

    visualization->SetAbsoluteTransform(visualization->GetAbsoluteLocation(), screenResolution);

    visualization->text = visualization->AddChild<UITextElement>(font, text, glm::vec2(10.0f, 50.0f), 50);
    visualization->text.lock()->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    visualization->text.lock()->SetColor(glm::vec4(0.75f, 0.75f, 0.75f, 0.75f));

    visualization->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return visualization;
}