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

    {
        constexpr auto textSize = 100;
        loading->displayText = loading->AddChild<UITextElement>(font, "Loading", glm::vec2(), textSize);
        auto text = loading->displayText.lock();
        text->anchorPoint = UIElement::AnchorPoint::eMiddle;
    }

    loading->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return loading;
}