#include "ui_menus.hpp"

#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "ui_button.hpp"
#include "ui_module.hpp"
#include "ui_text.hpp"
#include <resource_management/image_resource_manager.hpp>

std::unique_ptr<Canvas> CreateNavigationTestMenu(UIInputContext& uiInputContext, const glm::ivec2& canvasBounds, std::shared_ptr<GraphicsContext> graphicsContext)
{
    // resource loading.
    auto loadButtonStyle = [&graphicsContext]()
    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format
            = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        UIButton::ButtonStyle buttonStyle {
            .normalImage = graphicsContext->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_n.png")),
            .hoveredImage = graphicsContext->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_h.png")),
            .pressedImage = graphicsContext->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_p.png"))
        };
        return buttonStyle;
    };
    UIButton::ButtonStyle buttonStyle = loadButtonStyle();

    std::unique_ptr<Canvas> canvas = std::make_unique<Canvas>(canvasBounds);

    // temporary
    canvas->SetAbsoluteTransform(canvas->GetAbsoluteLocation(), canvas->GetRelativeScale());

    auto button1 = canvas->AddChild<UIButton>(buttonStyle, glm::vec2(0), glm::vec2(239, 36) * 2.0f).lock();
    button1->onMouseDownCallBack = []()
    { bblog::debug("b1 clicked"); };
    auto button2 = canvas->AddChild<UIButton>(buttonStyle, glm::vec2(0, 85), glm::vec2(239, 36) * 2.0f).lock();
    button2->onMouseDownCallBack = []()
    { bblog::debug("b2 clicked"); };

    button1->navigationTargets.down = button2;
    button1->navigationTargets.up = button2;

    button2->navigationTargets.down = button1;
    button2->navigationTargets.up = button1;

    graphicsContext->UpdateBindlessSet();

    uiInputContext.focusedUIElement = button1;
    return canvas;
}