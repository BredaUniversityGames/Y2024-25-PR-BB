#include "ui_mainMenu.hpp"
#include "ui_core.hpp"
#include "ui_text.hpp"
#include "vulkan_helper.hpp"


MainMenuCanvas::MainMenuCanvas(const glm::vec2& size, const VulkanBrain& brain, MAYBE_UNUSED ResourceHandle<Font> font)
    : Canvas(size)
{
    // demo scene
    auto sampler = util::CreateSampler(brain, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerAddressMode::eClampToBorder, vk::SamplerMipmapMode::eNearest, 0);

    ImageCreation buttonNormalImage;

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
    subButton->onMouseDownCallBack = []() {};
    subButton->onBeginHoverCallBack = [](){};
    
    std::unique_ptr<UIButton> playButton = std::make_unique<UIButton>();
    playButton->style = standardStyle;
    playButton->scale = { 300, 100 };
    playButton->SetLocation({ 100, 100 });
    playButton->onMouseDownCallBack = [&]()
    {
        subButton->visible = !subButton->visible;
    };
    playButton->onBeginHoverCallBack = [](){};


    AddChild(std::move(playButton));
    AddChild(std::move(subButton));
    
    UpdateChildAbsoluteLocations();
}
