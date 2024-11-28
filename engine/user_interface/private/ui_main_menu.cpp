#include "ui_main_menu.hpp"
#include "canvas.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "ui_text.hpp"
#include "vulkan_context.hpp"

MainMenuCanvas::MainMenuCanvas(const glm::vec2& size, MAYBE_UNUSED std::shared_ptr<GraphicsContext>& context, const std::shared_ptr<Font>& font)
    : Canvas(size)
{
    // demo scene

    CPUImage buttonNormalImage;

    buttonNormalImage.format
        = vk::Format::eR8G8B8A8Unorm;
    buttonNormalImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
    buttonNormalImage.ExtractDataFromPNG("assets/textures/button_background.png");
    buttonNormalImage.isHDR = false;
    auto normalImage = context->Resources()->ImageResourceManager().Create(buttonNormalImage);
    auto hoveredImage = context->Resources()->ImageResourceManager().Create(buttonNormalImage);

    UIButton::ButtonStyle standardStyle {
        .normalImage = normalImage,
        .hoveredImage = hoveredImage,
        .pressedImage = normalImage
    };

    std::unique_ptr<UIButton> subButton = std::make_unique<UIButton>();
    subButton->style = standardStyle;
    subButton->SetScale({ 300, 100 });
    subButton->SetLocation({ 100, 300 });
    subButton->onMouseDownCallBack = []() { };
    subButton->onBeginHoverCallBack = []() { };

    AddChild(std::move(subButton));

    std::unique_ptr<UIButton> playButton = std::make_unique<UIButton>();
    playButton->style = standardStyle;
    playButton->SetScale({ 300, 100 });
    playButton->SetLocation({ 100, 100 });
    playButton->onBeginHoverCallBack = []() { };
    playButton->onMouseDownCallBack = []() { };
    std::unique_ptr<UITextElement> playText = std::make_unique<UITextElement>(font);
    playText->text = "Play the Game";
    playText->SetScale({ 25, 25 });
    playText->SetLocation({ 810, 640 });
    playText->color = { 1, 1, 1, 1 };

    AddChild(std::move(playText));
    AddChild(std::move(playButton));
}
