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

#if 0
    {
        loading->displayText = loading->AddChild<UITextElement>(loading->_font.lock(), "Loading", glm::vec2(), _textSize);
        auto text = loading->displayText.lock();
        text->anchorPoint = UIElement::AnchorPoint::eMiddle;
    }
#endif

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
    while(true)
    {
        if(offset == std::string::npos)
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

    for(size_t i = 0; i < lines.size(); i++)
    {
        auto line = lines[i];
        auto textElement = _displayTexts[i].lock();
        if(!textElement)
        {
            _displayTexts[i] = this->AddChild<UITextElement>(_font.lock(), "", glm::vec2(), _textSize);
            textElement = _displayTexts[i].lock();
        }

        textElement->SetText(line);
        textElement->SetLocation(textElement->GetRelativeLocation() + glm::vec2(0.0f, i * _font.lock()->metrics.resolutionY - totalTextHeightOffset));
        textElement->SetColor(_displayTextColor);
    }
}

void LoadingScreen::SetDisplayTextColor(glm::vec4 color)
{
    _displayTextColor = color;
    for(const auto& element : _displayTexts)
    {
        if(element.lock())
        {
            element.lock()->SetColor(_displayTextColor);
        }
    }
}
