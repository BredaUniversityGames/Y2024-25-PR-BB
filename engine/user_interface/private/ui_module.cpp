#include "ui_module.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include <application_module.hpp>
#include <canvas.hpp>
#include <fonts.hpp>
#include <ui_button.hpp>
#include <ui_text.hpp>

// todo: move to scripting

void CreateQuitModal(Canvas& canvas, ResourceHandle<GPUImage> backgroundImage, UIButton::ButtonStyle buttonStyle, std::shared_ptr<UIFont>& font, std::function<void(void)> onExitButtonClick, std::function<void(void)> onReturnButtonClick)
{
    canvas.visibility = UIElement::VisibilityState::eNotUpdatedAndInvisble;

    UIImageElement& background = canvas.AddChild<UIImageElement>(backgroundImage);
    background.anchorPoint = UIElement::AnchorPoint::eFill;

    canvas.AddChild<UITextElement>(font, "are you sure?", glm::vec2(0, -60), 35);

    auto& yesButton = canvas.AddChild<UIButton>(buttonStyle, glm::vec2(-100, 0), glm::vec2(150, 60));
    yesButton.AddChild<UITextElement>(font, "yes");
    yesButton.onMouseDownCallBack = onExitButtonClick;

    auto& returnButton = canvas.AddChild<UIButton>(buttonStyle, glm::vec2(100, 0), glm::vec2(150, 60));
    returnButton.AddChild<UITextElement>(font, "no");
    returnButton.onMouseDownCallBack = onReturnButtonClick;
}

void UIModule::CreateMainMenu(std::shared_ptr<GraphicsContext> context, std::function<void(void)> onPlayButtonClick, MAYBE_UNUSED std::function<void(void)> onExitButtonClick)
{
    std::shared_ptr<UIFont> mainMenuFont = LoadFromFile("assets/fonts/Rooters.ttf", 48, context);

    CPUImage commonImageData;
    commonImageData.format
        = vk::Format::eR8G8B8A8Unorm;
    commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
    commonImageData.isHDR = false;

    UIButton::ButtonStyle buttonStyle {
        .normalImage = context->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_n.png")),
        .hoveredImage = context->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_h.png")),
        .pressedImage = context->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/wider2_p.png"))
    };

    UIElement& canvas = _viewport->AddElement<Canvas>(_viewport->GetExtend());
    canvas.SetAbsoluteTransform(canvas.GetAbsoluteLocation(), canvas.GetRelativeScale());

    auto backgroundImage = context->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/main_menu_bg.png"));
    auto& background = canvas.AddChild<UIImageElement>(backgroundImage);
    background.anchorPoint = UIElement::AnchorPoint::eFill;
    background.zLevel = -1;
    // ornaments
    ResourceHandle<GPUImage> ornamentImage = context->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/bottom_ornament.png"));
    canvas.AddChild<UIImageElement>(ornamentImage, glm::vec2(0, -85), glm::vec2(114, -45) * 2.0f);
    canvas.AddChild<UIImageElement>(ornamentImage, glm::vec2(0, 170), glm::vec2(114, 45) * 2.0f);

    // Play button
    auto& playButton = canvas.AddChild<UIButton>(buttonStyle, glm::vec2(0), glm::vec2(239, 36) * 2.0f);
    playButton.onMouseDownCallBack = onPlayButtonClick;
    playButton.AddChild<UITextElement>(mainMenuFont, "play");

    // Quit modal
    auto modalBackgroundImage = context->Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/ays_box.png"));

    // Quit button
    auto& openQuitModalButton = canvas.AddChild<UIButton>(buttonStyle, glm::vec2(0, 85), glm::vec2(239, 36) * 2.0f);
    openQuitModalButton.AddChild<UITextElement>(mainMenuFont, "quit");

    auto& modal = canvas.AddChild<Canvas>(glm::vec2(500, 300));
    CreateQuitModal(modal, modalBackgroundImage, buttonStyle, mainMenuFont, onExitButtonClick, [&]()
        { modal.visibility = UIElement::VisibilityState::eNotUpdatedAndInvisble; });

    openQuitModalButton.onMouseDownCallBack = [&]()
    { modal.visibility = UIElement::VisibilityState::eUpdatedAndVisible; };

    context->UpdateBindlessSet();
}

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    _viewport = std::make_unique<Viewport>(extend);
    return ModuleTickOrder::ePostTick;
}
void UIModule::Tick(Engine& engine)
{
    _viewport->Update(engine.GetModule<ApplicationModule>().GetInputManager());
}