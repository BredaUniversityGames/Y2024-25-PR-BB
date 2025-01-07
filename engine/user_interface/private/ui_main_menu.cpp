#include "ui_main_menu.hpp"

#include "fonts.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "ui_button.hpp"
#include "ui_module.hpp"
#include "ui_text.hpp"
#include <resource_management/image_resource_manager.hpp>
// todo: move to scripting
void CreateQuitModal(Canvas& canvas, ResourceHandle<GPUImage> backgroundImage, UIButton::ButtonStyle buttonStyle, std::shared_ptr<UIFont>& font, std::function<void(void)> onExitButtonClick, std::function<void(void)> onReturnButtonClick)
{
    canvas.visibility = UIElement::VisibilityState::eNotUpdatedAndInvisble;

    auto background = canvas.AddChild<UIImageElement>(backgroundImage);
    background.lock()->anchorPoint = UIElement::AnchorPoint::eFill;

    canvas.AddChild<UITextElement>(font, "are you sure?", glm::vec2(0, -60), 35);

    auto yesButton = canvas.AddChild<UIButton>(buttonStyle, glm::vec2(-100, 0), glm::vec2(150, 60));
    yesButton.lock()->AddChild<UITextElement>(font, "yes");
    yesButton.lock()->onMouseDownCallBack = onExitButtonClick;

    UINavigationMappings::ElementMap elementMapReturnButton = {};
    elementMapReturnButton.left = yesButton;

    auto returnButton = canvas.AddChild<UIButton>(buttonStyle, glm::vec2(100, 0), glm::vec2(150, 60), elementMapReturnButton);
    returnButton.lock()->AddChild<UITextElement>(font, "no");
    returnButton.lock()->onMouseDownCallBack = onReturnButtonClick;

    UINavigationMappings::ElementMap elementMapYesButton = {};
    elementMapYesButton.right = returnButton;
    yesButton.lock()->SetNavigationMappings(elementMapYesButton);
}

std::unique_ptr<Canvas> CreateMainMenuCanvas(UIInputContext& uiInputContext, const glm::ivec2& canvasBounds, std::shared_ptr<GraphicsContext> graphicsContext, std::function<void()> onPlayButtonClick, MAYBE_UNUSED std::function<void()> onExitButtonClick)
{

    std::shared_ptr<UIFont> mainMenuFont = LoadFromFile("assets/fonts/Rooters.ttf", 48, graphicsContext);

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

    std::unique_ptr<Canvas> canvas = std::make_unique<Canvas>(canvasBounds);
    canvas->SetAbsoluteTransform(canvas->GetAbsoluteLocation(), canvas->GetRelativeScale());

    auto backgroundImage = graphicsContext->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/main_menu_bg.png"));
    auto background = canvas->AddChild<UIImageElement>(backgroundImage);
    background.lock()->anchorPoint = UIElement::AnchorPoint::eFill;
    background.lock()->zLevel = -1;

    // ornaments
    ResourceHandle<GPUImage> ornamentImage = graphicsContext->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/bottom_ornament.png"));
    canvas->AddChild<UIImageElement>(ornamentImage, glm::vec2(0, -85), glm::vec2(114, -45) * 2.0f);
    canvas->AddChild<UIImageElement>(ornamentImage, glm::vec2(0, 170), glm::vec2(114, 45) * 2.0f);

    // Play button
    auto playButton = canvas->AddChild<UIButton>(buttonStyle, glm::vec2(0), glm::vec2(239, 36) * 2.0f);
    playButton.lock()->onMouseDownCallBack = onPlayButtonClick;
    playButton.lock()->AddChild<UITextElement>(mainMenuFont, "play");

    // Quit modal
    const auto& modalBackgroundImage = graphicsContext->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/ays_box.png"));

    // Quit button
    auto openQuitModalButton = canvas->AddChild<UIButton>(buttonStyle, glm::vec2(0, 85), glm::vec2(239, 36) * 2.0f);
    openQuitModalButton.lock()->AddChild<UITextElement>(mainMenuFont, "quit");

    UINavigationMappings::ElementMap OpenQuitModalButtonNavigationTargets = {};
    OpenQuitModalButtonNavigationTargets.up = playButton;
    OpenQuitModalButtonNavigationTargets.down = playButton;
    openQuitModalButton.lock()->SetNavigationMappings(OpenQuitModalButtonNavigationTargets);

    UINavigationMappings::ElementMap playButtonNavigationTargets = {};
    playButtonNavigationTargets.down = openQuitModalButton;
    playButtonNavigationTargets.up = openQuitModalButton;
    playButton.lock()->SetNavigationMappings(playButtonNavigationTargets);

    auto modal = canvas->AddChild<Canvas>(glm::vec2(500, 300));
    CreateQuitModal(*modal.lock(), modalBackgroundImage, buttonStyle, mainMenuFont, onExitButtonClick, [modal, openQuitModalButton]()
        { modal.lock()->visibility = UIElement::VisibilityState::eNotUpdatedAndInvisble; });

    auto modelnoButton = modal.lock()->GetChildren()[3];
    openQuitModalButton.lock()->onMouseDownCallBack = [modal, modelnoButton, openQuitModalButton]()
    {
        modal.lock()->visibility = UIElement::VisibilityState::eUpdatedAndVisible;
    };

    graphicsContext->UpdateBindlessSet();
    uiInputContext.focusedUIElement = playButton;

    return std::move(canvas);
}