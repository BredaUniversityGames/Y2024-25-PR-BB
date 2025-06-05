#include "engine.hpp"
#include "game_module.hpp"
#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"
#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_module.hpp"
#include "ui_slider.hpp"
#include "ui_text.hpp"
#include "ui_toggle.hpp"

std::shared_ptr<SettingsMenu> SettingsMenu::Create(
    Engine& engine,
    GraphicsContext& graphicsContext,
    const glm::uvec2& screenResolution,
    std::shared_ptr<UIFont> font)
{
    auto& gameModule = engine.GetModule<GameModule>();

    auto settings = std::make_shared<SettingsMenu>(screenResolution);

    settings->anchorPoint = UIElement::AnchorPoint::eMiddle;
    settings->SetAbsoluteTransform(settings->GetAbsoluteLocation(), screenResolution);

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
        auto image = settings->AddChild<UIImage>(backdropImage, glm::vec2(), glm::vec2());
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

    UIToggle::ToggleStyle toggleStyle {};

    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        toggleStyle.empty = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/toggle_empty.png"), sampler);
        toggleStyle.filled = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/toggle_full.png"), sampler);
    }

    UISlider::SliderStyle sliderStyle {};

    {
        // common image data.
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        sliderStyle.margin = 12.0f;
        sliderStyle.knobSize = glm::vec2 { 8, 8 } * 6.0f;
        sliderStyle.empty = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/slider_empty.png"), sampler);
        sliderStyle.filled = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/slider_full.png"), sampler);
        sliderStyle.knob = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/slider_knob.png"), sampler);
    }

    {
        // SETTINGS
        {
            settings->AddChild<UITextElement>(font, "Settings", glm::vec2 { 0.0f, -550.0f }, 100);
        }

        constexpr float textSize = 60;

        glm::vec2 elemPos = { 0.0f, -400.0f };
        constexpr glm::vec2 rowSize = { 1000.0f, 0.0f };
        constexpr glm::vec2 increment = { 0.0f, 100.0f };

        constexpr glm::vec2 buttonBaseSize = glm::vec2(87, 22) * 5.0f;
        constexpr glm::vec2 toggleSize = glm::vec2(16, 16) * 4.0f;

        constexpr glm::vec2 sliderSize = glm::vec2(128, 16) * 4.0f;
        constexpr glm::vec2 toggleOffset = glm::vec2(400.0f - sliderSize.x, -toggleSize.y * 0.25f + textSize * 0.25f);

        // Sensitivity
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Aim Sensitivity", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto slider = node->AddChild<UISlider>(sliderStyle, toggleOffset, sliderSize);
            slider->anchorPoint = UIElement::AnchorPoint::eTopRight;
            settings->sensitivitySlider = slider;

            auto callback = [&gameModule](float val)
            { gameModule.GetSettings().aimSensitivity = val; };

            slider->value = gameModule.GetSettings().aimSensitivity;
            slider->OnSlide(callback);
        }

        // Fov
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "FOV", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto slider = node->AddChild<UISlider>(sliderStyle, toggleOffset, sliderSize);
            slider->anchorPoint = UIElement::AnchorPoint::eTopRight;
            settings->fovSlider = slider;

            auto callback = [&gameModule](float val)
            { gameModule.GetSettings().fov = val; };

            slider->value = gameModule.GetSettings().fov;
            slider->OnSlide(callback);
        }

        // Aim Assist
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Aim Assist", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto toggle = node->AddChild<UIToggle>(toggleStyle, toggleOffset + glm::vec2(sliderSize.x - toggleSize.x, 0), toggleSize);
            toggle->anchorPoint = UIElement::AnchorPoint::eTopRight;
            settings->aimAssistToggle = toggle;

            auto callback = [&gameModule](bool val)
            { gameModule.GetSettings().aimAssist = val; };

            toggle->state = gameModule.GetSettings().aimAssist;
            toggle->OnToggle(callback);
        }

        // Gamma
        // {
        //     auto node = settings->AddChild<Canvas>(rowSize);
        //     node->SetLocation(elemPos);
        //     elemPos += increment;

        //     auto text = node->AddChild<UITextElement>(font, "Screen brightness", glm::vec2(), textSize);
        //     text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

        //     auto slider = node->AddChild<UISlider>(sliderStyle, toggleOffset, sliderSize);
        //     slider->anchorPoint = UIElement::AnchorPoint::eTopRight;
        //     settings->gammaSlider = slider;

        //     auto callback = [&gameModule](float val)
        //     { gameModule.GetSettings().gammaSlider = val; };

        //     slider->value = gameModule.GetSettings().gammaSlider;
        //     slider->OnSlide(callback);
        // }

        // VSync
        // {
        //     auto node = settings->AddChild<Canvas>(rowSize);
        //     node->SetLocation(elemPos);
        //     elemPos += increment;

        //     auto text = node->AddChild<UITextElement>(font, "Toggle VSync", glm::vec2(), textSize);
        //     text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

        //     auto toggle = node->AddChild<UIToggle>(toggleStyle, toggleOffset+ glm::vec2(sliderSize.x-toggleSize.x,0), toggleSize);
        //     toggle->anchorPoint = UIElement::AnchorPoint::eTopRight;
        //     settings->vsyncToggle = toggle;

        //     auto callback = [&gameModule](bool val)
        //     { gameModule.GetSettings().vsync = val; };

        //     toggle->state = gameModule.GetSettings().vsync;
        //     toggle->OnToggle(callback);
        // }

        // Master Volume
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Master Volume", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto slider = node->AddChild<UISlider>(sliderStyle, toggleOffset, sliderSize);
            slider->anchorPoint = UIElement::AnchorPoint::eTopRight;
            settings->masterVolume = slider;

            auto callback = [&gameModule](float val)
            { gameModule.GetSettings().masterVolume = val; };

            slider->value = gameModule.GetSettings().masterVolume;
            slider->OnSlide(callback);
        }

        // Music Volume
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Music Volume", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto slider = node->AddChild<UISlider>(sliderStyle, toggleOffset, sliderSize);
            slider->anchorPoint = UIElement::AnchorPoint::eTopRight;
            settings->musicVolume = slider;

            auto callback = [&gameModule](float val)
            { gameModule.GetSettings().musicVolume = val; };

            slider->value = gameModule.GetSettings().musicVolume;
            slider->OnSlide(callback);
        }

        // SFX Volume
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "SFX Volume", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto slider = node->AddChild<UISlider>(sliderStyle, toggleOffset, sliderSize);
            slider->anchorPoint = UIElement::AnchorPoint::eTopRight;
            settings->sfxVolume = slider;

            auto callback = [&gameModule](float val)
            { gameModule.GetSettings().sfxVolume = val; };

            slider->value = gameModule.GetSettings().sfxVolume;
            slider->OnSlide(callback);
        }

        // VSYNC
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Toggle Vsync", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto toggle = node->AddChild<UIToggle>(toggleStyle, toggleOffset + glm::vec2(sliderSize.x - toggleSize.x, 0), toggleSize);
            toggle->anchorPoint = UIElement::AnchorPoint::eTopRight;

            toggle->state = gameModule.GetSettings().vsync;

            auto callback = [&gameModule](bool val)
            { gameModule.GetSettings().vsync = val; };

            toggle->state = gameModule.GetSettings().vsync;
            toggle->OnToggle(callback);
            settings->vsyncToggle = toggle;
        }

        // FRAME COUNTER
        {
            auto node = settings->AddChild<Canvas>(rowSize);
            node->SetLocation(elemPos);
            elemPos += increment;

            auto text = node->AddChild<UITextElement>(font, "Toggle Framerate Display", glm::vec2(), textSize);
            text->anchorPoint = UIElement::AnchorPoint::eTopLeft;

            auto toggle = node->AddChild<UIToggle>(toggleStyle, toggleOffset + glm::vec2(sliderSize.x - toggleSize.x, 0), toggleSize);
            toggle->anchorPoint = UIElement::AnchorPoint::eTopRight;

            toggle->state = gameModule.GetSettings().framerateCounter;

            auto callback = [&gameModule](bool val)
            { gameModule.GetSettings().framerateCounter = val; };

            toggle->state = gameModule.GetSettings().framerateCounter;
            toggle->OnToggle(callback);
            settings->fpsToggle = toggle;
        }

        // BACK BUTTON
        {
            constexpr glm::vec2 backPos = { 0.0f, 500.0f };
            auto back = settings->AddChild<UIButton>(buttonStyle, backPos, buttonBaseSize);
            back->anchorPoint = UIElement::AnchorPoint::eMiddle;
            back->AddChild<UITextElement>(font, "Back", textSize);

            auto callback = [&engine]()
            {
                auto& gameModule = engine.GetModule<GameModule>();
                engine.GetModule<UIModule>().uiInputContext.focusedUIElement = gameModule.PopPreviousFocusedElement();
                gameModule.GetSettings().SaveToFile(GAME_SETTINGS_FILE);
                gameModule.PopUIMenu();
            };

            back->OnPress(Callback { callback });
            settings->backButton = back;
        }
    }

    // UI Nav
    {
        settings->sensitivitySlider.lock()->navigationTargets.up = settings->backButton;
        settings->sensitivitySlider.lock()->navigationTargets.down = settings->aimAssistToggle;

        settings->aimAssistToggle.lock()->navigationTargets.up = settings->sensitivitySlider;
        settings->aimAssistToggle.lock()->navigationTargets.down = settings->masterVolume;

        // settings->gammaSlider.lock()->navigationTargets.up = settings->aimAssistToggle;
        // settings->gammaSlider.lock()->navigationTargets.down = settings->vsyncToggle;

        settings->masterVolume.lock()->navigationTargets.up = settings->aimAssistToggle;
        settings->masterVolume.lock()->navigationTargets.down = settings->musicVolume;

        settings->musicVolume.lock()->navigationTargets.up = settings->masterVolume;
        settings->musicVolume.lock()->navigationTargets.down = settings->sfxVolume;

        settings->sfxVolume.lock()->navigationTargets.up = settings->musicVolume;
        settings->sfxVolume.lock()->navigationTargets.down = settings->vsyncToggle;

        settings->vsyncToggle.lock()->navigationTargets.up = settings->sfxVolume;
        settings->vsyncToggle.lock()->navigationTargets.down = settings->fpsToggle;

        settings->fpsToggle.lock()->navigationTargets.up = settings->vsyncToggle;
        settings->fpsToggle.lock()->navigationTargets.down = settings->backButton;

        settings->backButton.lock()->navigationTargets.up = settings->fpsToggle;
        settings->backButton.lock()->navigationTargets.down = settings->sensitivitySlider;
    }

    settings->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return settings;
}
