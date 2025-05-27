#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

std::shared_ptr<LoadingScreen> LoadingScreen::Create(GraphicsContext& graphicsContext, const glm::uvec2& screenResolution, std::shared_ptr<UIFont> font)
{
    auto loading = std::make_shared<LoadingScreen>(screenResolution);

    loading->_font = font;

    loading->anchorPoint = UIElement::AnchorPoint::eMiddle;
    loading->SetAbsoluteTransform(loading->GetAbsoluteLocation(), screenResolution);

    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;
        commonImageData.name = "BlackBackdrop";

        constexpr std::byte black = {};
        constexpr std::byte white = static_cast<std::byte>(255);
        commonImageData.initialData = { black, black, black, white };

        auto backdropImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData);
        auto image = loading->AddChild<UIImage>(backdropImage, glm::vec2(), glm::vec2());
        image->anchorPoint = UIElement::AnchorPoint::eFill;
    }

    loading->_continueText = loading->AddChild<UITextElement>(loading->_font.lock(), "Press E to continue", glm::vec2(), _textSize / 2.0f);
    std::shared_ptr<UITextElement> contText = loading->_continueText.lock();
    contText->anchorPoint = UIElement::AnchorPoint::eBottomRight;

    loading->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return loading;
}

void LoadingScreen::SetDisplayText(std::string text)
{
    // Split text into lines when needed
    std::array<std::string, MAX_LINE_BREAKS> lines;
    lines.fill("");

    uint32_t i = 0;
    size_t offset = text.find("\n");
    while (true)
    {
        if (offset == std::string::npos)
        {
            lines[i] = text;
            break;
        }

        std::string substr = text.substr(0, offset);
        lines[i++] = substr;

        text = text.substr(offset + 1);
        offset = text.find("\n");
    }

    float totalTextHeightOffset = (static_cast<float>(_font.lock()->metrics.resolutionY) * static_cast<float>(i)) / 2.0f;

    for (size_t i = 0; i < lines.size(); i++)
    {
        auto line = lines[i];
        auto textElement = _displayTexts[i].lock();
        if (!textElement)
        {
            _displayTexts[i] = this->AddChild<UITextElement>(_font.lock(), "", glm::vec2(), _textSize);
            textElement = _displayTexts[i].lock();
        }

        textElement->SetText(line);
        textElement->SetLocation(glm::vec2(0.0f, i * _font.lock()->metrics.resolutionY - totalTextHeightOffset));
        textElement->display_color = _displayTextColor;
    }

    //_continueText.lock()->SetLocation(glm::vec2(0.0f, totalTextHeightOffset + static_cast<float>(_font.lock()->metrics.resolutionY)));
    _continueText.lock()->SetLocation(glm::vec2(30.0f, 30.0f));

    UpdateAllChildrenAbsoluteTransform();
}

void LoadingScreen::SetDisplayTextColor(glm::vec4 color)
{
    _displayTextColor = color;
    for (const auto& element : _displayTexts)
    {
        if (element.lock())
        {
            element.lock()->display_color = _displayTextColor;
        }
    }
}

void LoadingScreen::ShowContinuePrompt()
{
    _continueText.lock()->display_color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

void LoadingScreen::HideContinuePrompt()
{
    _continueText.lock()->display_color = { 1.0f, 1.0f, 1.0f, 0.0f };
}