#include "fonts.hpp"
#include "graphics_context.hpp"
#include "ui/ui_menus.hpp"
#include "ui_text.hpp"

std::shared_ptr<FrameCounter> FrameCounter::Create(const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font)
{
    auto counter = std::make_shared<FrameCounter>(screenResolution);

    counter->SetAbsoluteTransform(counter->GetAbsoluteLocation(), screenResolution);

    counter->text = counter->AddChild<UITextElement>(font, "", glm::vec2(160.0f, 80.0f), 70);
    counter->text.lock()->anchorPoint = UIElement::AnchorPoint::eTopRight;
    counter->text.lock()->display_color = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);

    counter->UpdateAllChildrenAbsoluteTransform();
    return counter;
}

void FrameCounter::SetVal(float fps)
{
    constexpr float BIAS = 0.05f;

    if (runningAverage == 0.0f)
        runningAverage = fps;

    runningAverage = runningAverage * (1.0f - BIAS) + fps * BIAS;
    text.lock()->SetText(fmt::format("{}", static_cast<uint32_t>(runningAverage)));
}