#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "game_actions.hpp"
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
    constexpr float buttonTextSize = 60;

    auto backButton = menu->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
    backButton->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    backButton->AddChild<UITextElement>(font, "Back", buttonTextSize)->SetColor(glm::vec4(1, 1, 1, 1));
    menu->backButton = backButton;

    auto actionsPanel = menu->AddChild<Canvas>(screenResolution);

    {
        actionsPanel->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        actionsPanel->SetLocation(glm::vec2(0.0f, 0.0f));
    }

    constexpr float actionSetTextSize = 60;
    constexpr float actionTextSize = 50;
    constexpr glm::vec2 increment = { 0.0f, 60.0f };
    float actionSetheightLocation = 35.0f;

    for (const ActionSet& actionSet : GAME_ACTIONS)
    {
        // We need to change the active action set to be able to retrieve the wanted controller glyph
        actionManager.SetActiveActionSet(actionSet.name);

        ActionSetControls& set = menu->actionSetControls.emplace_back();
        set.canvas = actionsPanel->AddChild<Canvas>(glm::vec2{ 0.0f, 0.0f });
        set.canvas->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        set.canvas->SetLocation(glm::vec2(20.0f, actionSetheightLocation));

        set.nameText = set.canvas->AddChild<UITextElement>(font, actionSet.name, actionSetTextSize);
        set.nameText->SetColor(glm::vec4(1, 1, 1, 1));
        set.nameText->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        set.nameText->SetLocation({ 0.0f, 0.0f });

        float actionHeightLocation = actionSetTextSize + 10.0f;

        // TODO: Add text for each input as well
        // TODO: Show all possible inputs for an action

        for (const AnalogAction& analogAction : actionSet.analogActions)
        {
            ActionControls& action = set.actionControls.emplace_back();
            action.canvas = set.canvas->AddChild<Canvas>(glm::vec2 { screenResolution.x, increment.y });
            action.canvas->anchorPoint = UIElement::AnchorPoint::eTopLeft;
            action.canvas->SetLocation(glm::vec2(0.0f, actionHeightLocation));

            actionHeightLocation += increment.y;

            action.nameText = action.canvas->AddChild<UITextElement>(font, analogAction.name, actionTextSize);
            action.nameText->SetColor(glm::vec4(0.8f, 0.8f, 0.8f, 1));
            action.nameText->anchorPoint = UIElement::AnchorPoint::eTopLeft;
            action.nameText->SetLocation({ 0.0f, 0.0f });

            constexpr float glyphHorizontalMargin = 5.0f;
            float glyphHorizontalPosition = screenResolution.x / 6.0f;

            const std::vector<GamepadOriginVisual> gamepadOrigins = actionManager.GetAnalogActionGamepadOriginVisual(analogAction.name);

            for (const GamepadOriginVisual& origin : gamepadOrigins)
            {
                ActionControls::Binding& binding = action.bindings.emplace_back();

                CPUImage glyphCPUImage;
                glyphCPUImage.format = vk::Format::eR8G8B8A8Unorm;
                glyphCPUImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
                glyphCPUImage.FromPNG(origin.glyphImagePath);
                ResourceHandle<GPUImage> glyphImage = graphicsContext.Resources()->ImageResourceManager().Create(glyphCPUImage, sampler);

                glm::vec2 size = glm::vec2(glyphCPUImage.width, glyphCPUImage.height) * 0.15f;
                binding.glyph = action.canvas->AddChild<UIImage>(glyphImage, glm::vec2(glyphHorizontalPosition, size.y / 2.0f * -1.0f), size);
                binding.glyph->anchorPoint = UIElement::AnchorPoint::eTopLeft;

                glyphHorizontalPosition += size.x + glyphHorizontalMargin;
            }
        }

        for (const DigitalAction& digitalAction : actionSet.digitalActions)
        {
            ActionControls& action = set.actionControls.emplace_back();
            action.canvas = set.canvas->AddChild<Canvas>(glm::vec2 { screenResolution.x, increment.y });
            action.canvas->anchorPoint = UIElement::AnchorPoint::eTopLeft;
            action.canvas->SetLocation(glm::vec2(0.0f, actionHeightLocation));

            actionHeightLocation += increment.y;

            action.nameText = action.canvas->AddChild<UITextElement>(font, digitalAction.name, actionTextSize);
            action.nameText->SetColor(glm::vec4(0.8f, 0.8f, 0.8f, 1));
            action.nameText->anchorPoint = UIElement::AnchorPoint::eTopLeft;
            action.nameText->SetLocation({ 0.0f, 0.0f });

            constexpr float glyphHorizontalMargin = 25.0f;
            float glyphHorizontalPosition = screenResolution.x / 6.0f;

            const std::vector<GamepadOriginVisual> gamepadOrigins = actionManager.GetDigitalActionGamepadOriginVisual(digitalAction.name);

            for (const GamepadOriginVisual& origin : gamepadOrigins)
            {
                ActionControls::Binding& binding = action.bindings.emplace_back();

                CPUImage glyphCPUImage;
                glyphCPUImage.format = vk::Format::eR8G8B8A8Unorm;
                glyphCPUImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
                glyphCPUImage.FromPNG(origin.glyphImagePath);
                ResourceHandle<GPUImage> glyphImage = graphicsContext.Resources()->ImageResourceManager().Create(glyphCPUImage, sampler);

                glm::vec2 size = glm::vec2(glyphCPUImage.width, glyphCPUImage.height) * 0.15f;
                binding.glyph = action.canvas->AddChild<UIImage>(glyphImage, glm::vec2(glyphHorizontalPosition, size.y / 2.0f * -1.0f), size);
                binding.glyph->anchorPoint = UIElement::AnchorPoint::eTopLeft;

                glyphHorizontalPosition += size.x + glyphHorizontalMargin;
            }
        }

        actionSetheightLocation += actionHeightLocation + 20.0f;
    }

    // Make sure to go back to the user interface action set
    actionManager.SetActiveActionSet("UserInterface");

    menu->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return menu;
}
