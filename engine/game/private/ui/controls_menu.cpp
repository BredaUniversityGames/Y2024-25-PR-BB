#include "ui/ui_menus.hpp"
#include "ui_image.hpp"
#include "ui_text.hpp"

#include "graphics_context.hpp"
#include "graphics_resources.hpp"
#include "game_actions.hpp"
#include "resource_management/image_resource_manager.hpp"
#include "resource_management/sampler_resource_manager.hpp"

std::shared_ptr<ControlsMenu> ControlsMenu::Create(const glm::uvec2 &screenResolution, GraphicsContext &graphicsContext, ActionManager &actionManager, std::shared_ptr<UIFont> font)
{
    auto menu = std::make_shared<ControlsMenu>(screenResolution, graphicsContext, actionManager, font);
    menu->anchorPoint = UIElement::AnchorPoint::eMiddle;
    menu->SetAbsoluteTransform(menu->GetAbsoluteLocation(), screenResolution);

    SamplerCreation samplerCreation;
    samplerCreation.minFilter = vk::Filter::eNearest;
    samplerCreation.magFilter = vk::Filter::eNearest;
    menu->sampler = graphicsContext.Resources()->SamplerResourceManager().Create(samplerCreation);

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

        buttonStyle.normalImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button.png"), menu->sampler);
        buttonStyle.hoveredImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_2.png"), menu->sampler);
        buttonStyle.pressedImage = graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/button_selected.png"), menu->sampler);
    }

    glm::vec2 buttonPos = { 50.0f, 100.0f };
    constexpr glm::vec2 buttonBaseSize = glm::vec2(87, 22) * 3.0f;
    constexpr float buttonTextSize = 60.0f;

    auto backButton = menu->AddChild<UIButton>(buttonStyle, buttonPos, buttonBaseSize);
    backButton->anchorPoint = UIElement::AnchorPoint::eBottomLeft;
    backButton->AddChild<UITextElement>(font, "Back", buttonTextSize)->SetColor(glm::vec4(1, 1, 1, 1));
    menu->backButton = backButton;

    auto popupPanel = menu->AddChild<Canvas>(screenResolution);
    popupPanel->anchorPoint = UIElement::AnchorPoint::eMiddle;
    popupPanel->SetLocation(glm::vec2(0.0f, -80.0f));

    {
        CPUImage commonImageData;
        commonImageData.format = vk::Format::eR8G8B8A8Unorm;
        commonImageData.SetFlags(vk::ImageUsageFlagBits::eSampled);
        commonImageData.isHDR = false;

        auto image = popupPanel->AddChild<UIImage>(graphicsContext.Resources()->ImageResourceManager().Create(commonImageData.FromPNG("assets/textures/ui/popup_background.png"), menu->sampler), glm::vec2(0.0f), glm::vec2(0.0f));
        image->anchorPoint = UIElement::AnchorPoint::eMiddle;
        image->SetScale({screenResolution.x * 1.2f, screenResolution.y * 1.2f });
    }

    menu->actionsPanel = popupPanel->AddChild<Canvas>(screenResolution);
    menu->UpdateBindings();

    return menu;
}

void ControlsMenu::UpdateBindings()
{
    ClearBindings();

    constexpr float actionSetTextSize = 60.0f;
    constexpr float actionHeightMarginY = actionSetTextSize + 10.0f;
    float actionSetheightLocation = 35.0f;
    float actionHeightLocation = actionHeightMarginY;
    constexpr float heightIncrement = 60.0f;

    for (const ActionSet& actionSet : GAME_ACTIONS)
    {
        // We need to change the active action set to be able to retrieve the wanted controller glyph
        _actionManager.SetActiveActionSet(actionSet.name);

        ActionSetControls& set = actionSetControls.emplace_back();
        set.canvas = actionsPanel->AddChild<Canvas>(glm::vec2{ 0.0f, 0.0f });
        set.canvas->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        set.canvas->SetLocation(glm::vec2(20.0f, actionSetheightLocation));

        set.nameText = set.canvas->AddChild<UITextElement>(_font, actionSet.name, actionSetTextSize);
        set.nameText->SetColor(glm::vec4(1, 1, 1, 1));
        set.nameText->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        set.nameText->SetLocation({ 0.0f, 0.0f });

        for (const AnalogAction& analogAction : actionSet.analogActions)
        {
            set.actionControls.push_back(AddActionVisualization(analogAction.name, *set.canvas, actionHeightLocation, true));
            actionHeightLocation += heightIncrement;
        }

        for (const DigitalAction& digitalAction : actionSet.digitalActions)
        {
            set.actionControls.push_back(AddActionVisualization(digitalAction.name, *set.canvas, actionHeightLocation, false));
            actionHeightLocation += heightIncrement;
        }

        actionSetheightLocation += actionHeightLocation + 20.0f;
        actionHeightLocation = actionHeightMarginY;
    }

    // Make sure to go back to the user interface action set
    _actionManager.SetActiveActionSet("UserInterface");

    UpdateAllChildrenAbsoluteTransform();
    _graphicsContext.UpdateBindlessSet();
}

ControlsMenu::ActionControls ControlsMenu::AddActionVisualization(const std::string& actionName, Canvas& parent, float positionY, bool isAnalogInput)
{
    constexpr float actionTextSize = 50.0f;
    constexpr float actionOriginBindingTextSize = 40.0f;
    constexpr float glyphHorizontalMargin = 25.0f;
    constexpr float originHorizontalMargin = 200.0f;
    constexpr float actionOriginBindingTextMarginMultiplier = 12.0f;
    constexpr float canvasScaleY = actionTextSize + 10.0f;

    ActionControls action {};
    action.canvas = parent.AddChild<Canvas>(glm::vec2 { _screenResolution.x, canvasScaleY });
    action.canvas->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    action.canvas->SetLocation(glm::vec2(0.0f, positionY));

    action.nameText = action.canvas->AddChild<UITextElement>(_font, actionName, actionTextSize);
    action.nameText->SetColor(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
    action.nameText->anchorPoint = UIElement::AnchorPoint::eTopLeft;
    action.nameText->SetLocation({ 0.0f, 0.0f });

    const std::vector<GamepadOriginVisual> gamepadOrigins = isAnalogInput ? _actionManager.GetAnalogActionGamepadOriginVisual(actionName) : _actionManager.GetDigitalActionGamepadOriginVisual(actionName);
    float horizontalOffset = _screenResolution.x / 6.0f;

    for (const GamepadOriginVisual& origin : gamepadOrigins)
    {
        ActionControls::Binding& binding = action.bindings.emplace_back();

        // Create binding text
        binding.originName = action.canvas->AddChild<UITextElement>(_font, origin.bindingInputName, actionOriginBindingTextSize);
        binding.originName->SetColor(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
        binding.originName->anchorPoint = UIElement::AnchorPoint::eTopLeft;
        binding.originName->SetLocation({ horizontalOffset, 0.0f });

        horizontalOffset += binding.originName->GetAbsoluteScale().x * actionOriginBindingTextSize * origin.bindingInputName.length() + glyphHorizontalMargin * actionOriginBindingTextMarginMultiplier;

        // Create glyph
        ResourceHandle<GPUImage> glyphImage = GetGlyphImage(origin.glyphImagePath);
        const GPUImage* gpuImage = _graphicsContext.Resources()->ImageResourceManager().Access(glyphImage);

        glm::vec2 size = glm::vec2(gpuImage->width, gpuImage->height) * 0.15f;
        binding.glyph = action.canvas->AddChild<UIImage>(glyphImage, glm::vec2(horizontalOffset, 0.0f), size);
        binding.glyph->anchorPoint = UIElement::AnchorPoint::eTopLeft;

        horizontalOffset += size.x + originHorizontalMargin;
    }

    return action;
}

ResourceHandle<GPUImage> ControlsMenu::GetGlyphImage(const std::string& path)
{
    auto it = _glyphsCache.find(path);
    if (it != _glyphsCache.end())
    {
        return it->second;
    }

    CPUImage glyphCPUImage;
    glyphCPUImage.format = vk::Format::eR8G8B8A8Unorm;
    glyphCPUImage.SetFlags(vk::ImageUsageFlagBits::eSampled);
    glyphCPUImage.FromPNG(path);

    _glyphsCache[path] = _graphicsContext.Resources()->ImageResourceManager().Create(glyphCPUImage, sampler);
    return _glyphsCache[path];
}

void ControlsMenu::ClearBindings()
{
    // Manually clean up, as there is no remove child function yet
    for (ActionSetControls& set : actionSetControls)
    {
        set.canvas.reset();
        set.nameText.reset();

        for (ActionControls& action : set.actionControls)
        {
            action.canvas.reset();
            action.nameText.reset();

            for (ActionControls::Binding& binding : action.bindings)
            {
                binding.originName.reset();
                binding.originName.reset();
            }
        }
    }

    actionsPanel->GetChildren().clear();
    actionSetControls.clear();
}
