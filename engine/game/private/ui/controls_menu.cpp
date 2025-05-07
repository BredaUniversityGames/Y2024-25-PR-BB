#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"

std::shared_ptr<ControlsMenu> ControlsMenu::Create(GraphicsContext &graphicsContext, ActionManager &actionManager, const glm::uvec2 &screenResolution, std::shared_ptr<UIFont> font)
{
    auto menu = std::make_shared<ControlsMenu>(screenResolution);
    menu->anchorPoint = UIElement::AnchorPoint::eMiddle;
    menu->SetAbsoluteTransform(menu->GetAbsoluteLocation(), screenResolution);

    SamplerCreation samplerCreation;
    samplerCreation.minFilter = vk::Filter::eNearest;
    samplerCreation.magFilter = vk::Filter::eNearest;
    static ResourceHandle<Sampler> sampler = graphicsContext.Resources()->SamplerResourceManager().Create(samplerCreation);

    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;
        commonImageData.name = "BlackBackdrop";

        constexpr std::byte black = {};
        constexpr std::byte transparent = static_cast<std::byte>(150);
        commonImageData.initialData = { black, black, black, transparent };

        auto backdropImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData);
        auto image = menu->AddChild<UIImage>(backdropImage, glm::vec2(), glm::vec2());
        image->anchorPoint = UIElement::AnchorPoint::eFill;
    }

    // Button Style
    UIButton::ButtonStyle buttonStyle {};
    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        buttonStyle.normalImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button.png"), sampler);
        buttonStyle.hoveredImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_2.png"), sampler);
        buttonStyle.pressedImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_selected.png"), sampler);
    }

    glm::vec2 buttonPos = { 50.0f, 100.0f };
    constexpr glm::vec2 buttonBaseSize = glm::vec2(87, 22) * 3.0f;
    constexpr float textSize = 60;

    auto backButton = menu->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
    backButton->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    backButton->AddChild<UITextElement>(font, "Back", textSize)->SetColor(glm::vec4(1, 1, 1, 1));
    menu->backButton = backButton;

    menu->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return menu;
}
