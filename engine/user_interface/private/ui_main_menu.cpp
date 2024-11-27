#include "ui_main_menu.hpp"
#include "graphics_context.hpp"
#include "ui_core.hpp"
#include "ui_text.hpp"

MainMenuCanvas::MainMenuCanvas(const glm::vec2& size, MAYBE_UNUSED std::shared_ptr<GraphicsContext> context, const std::shared_ptr<Font>& font)
    : Canvas(size)
{
    // demo scene

    /*ImageCreation buttonNormalImage;

    buttonNormalImage.format
        = vk::Format::eR8G8B8A8Unorm;
    buttonNormalImage.sampler = sampler.get();
    buttonNormalImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
    buttonNormalImage.LoadFromFile("assets/textures/buttonBackGround.png");
    buttonNormalImage.isHDR = false;

    auto normalImage = brain.GetImageResourceManager().Create(buttonNormalImage);

    buttonNormalImage.LoadFromFile("assets/textures/buttonHovered.png");
    auto hoveredImage = brain.GetImageResourceManager().Create(buttonNormalImage);

    UIButton::ButtonStyle standardStyle {
        .normalImage = normalImage,
        .hoveredImage = hoveredImage,
        .pressedImage = normalImage
    };

    brain.UpdateBindlessSet();

    std::unique_ptr<UIButton> subButton = std::make_unique<UIButton>();
    subButton->style = standardStyle;
    subButton->scale = { 300, 100 };
    subButton->SetLocation({ 100, 300 });
    subButton->onMouseDownCallBack = []() { };
    subButton->onBeginHoverCallBack = []() { };

    std::unique_ptr<UIButton> playButton = std::make_unique<UIButton>();
    playButton->style = standardStyle;
    playButton->scale = { 300, 100 };
    playButton->SetLocation({ 100, 100 });
    playButton->onMouseDownCallBack = [&]()
    {
        subButton->visible = !subButton->visible;
    };
    playButton->onBeginHoverCallBack = []() { };*/

    std::unique_ptr<UITextElement> playText = std::make_unique<UITextElement>(font);
    playText->text = "Play the Game";
    playText->scale = { 1, 1 };
    playText->SetLocation({ 810, 640 });
    playText->color = { 0, 1, 0, 1 };

    AddChild(std::move(playText));

    // AddChild(std::move(playButton));
    // AddChild(std::move(subButton));

    UpdateChildAbsoluteLocations();
}
