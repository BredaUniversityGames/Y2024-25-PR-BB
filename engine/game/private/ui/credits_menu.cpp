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

std::shared_ptr<CreditsMenu> CreditsMenu::Create(
    Engine& engine,
    GraphicsContext& graphicsContext,
    const glm::uvec2& screenResolution,
    std::shared_ptr<UIFont> font)
{
    auto credits = std::make_shared<CreditsMenu>(screenResolution);

    credits->anchorPoint = UIElement::AnchorPoint::eMiddle;
    credits->SetAbsoluteTransform(credits->GetAbsoluteLocation(), screenResolution);

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
        auto image = credits->AddChild<UIImage>(backdropImage, glm::vec2(), glm::vec2());
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

    {
        // Title
        {
            credits->AddChild<UITextElement>(font, "The Bubonic Brotherhood", glm::vec2 { 0.0f, -575.0f }, 100);
        }

        constexpr float textSize = 55;

        glm::vec2 elemPos = { 0.0f, -550.0f };
        constexpr glm::vec2 rowSize = { 900.0f, 0.0f };
        constexpr glm::vec2 increment = { 0.0f, 80.0f };

        constexpr glm::vec2 buttonBaseSize = glm::vec2(87, 22) * 5.0f;

        std::pair<const char*, const char*> bubonic_brotherhood[] = {
            { "Ferri de Lange", "Lead / Graphics" },
            { "Pawel Trajdos", "Producer / Designer" },
            { "Luuk Asmus", "Gameplay / Engine" },
            { "Leo Hellmig", "Gameplay / Engine" },
            { "Lon van Ettinger", "Graphics / QA" },
            { "Marcin Zalewski", "Engine / Graphics" },
            { "Rares Dumitru", "Gameplay / Graphics" },
            { "Lars Janssen", "Engine / Trailer" },
            { "Sikandar Maksoedan", "Setdressing / Tech art" },
            { "Vengar van de Wal", "Graphics" },
            { "Santiago Bernardino", "Engine / QA" },
        };

        {
            for (auto& elem : bubonic_brotherhood)
            {
                elemPos += increment;

                auto node = credits->AddChild<Canvas>(rowSize);
                node->SetLocation(elemPos);

                auto name1 = node->AddChild<UITextElement>(font, elem.first, glm::vec2(), textSize);
                name1->anchorPoint = UIElement::AnchorPoint::eTopLeft;

                auto name2 = node->AddChild<UITextElement>(font, elem.second, glm::vec2(), textSize);
                name2->anchorPoint = UIElement::AnchorPoint::eTopRight;
            }
        }

        // // Gameplay
        // {
        //     credits->AddChild<UITextElement>(font, "Gameplay", elemPos, textSize);
        //     elemPos += increment;

        //     auto node = credits->AddChild<Canvas>(rowSize);
        //     node->SetLocation(elemPos);
        //     elemPos += increment;

        //     auto name1 = node->AddChild<UITextElement>(font, "Name 1", glm::vec2(), textSize);
        //     name1->anchorPoint = UIElement::AnchorPoint::eTopLeft;

        //     auto name2 = node->AddChild<UITextElement>(font, "Name 2", glm::vec2(), textSize);
        //     name2->anchorPoint = UIElement::AnchorPoint::eTopRight;
        // }

        // // Engine
        // {
        //     credits->AddChild<UITextElement>(font, "Engine", elemPos, textSize);
        //     elemPos += increment;

        //     auto node = credits->AddChild<Canvas>(rowSize);
        //     node->SetLocation(elemPos);
        //     elemPos += increment;

        //     auto name1 = node->AddChild<UITextElement>(font, "Name 3", glm::vec2(), textSize);
        //     name1->anchorPoint = UIElement::AnchorPoint::eTopLeft;

        //     auto name2 = node->AddChild<UITextElement>(font, "Name 4", glm::vec2(), textSize);
        //     name2->anchorPoint = UIElement::AnchorPoint::eTopRight;
        // }

        // // Graphics
        // {
        //     credits->AddChild<UITextElement>(font, "Graphics", elemPos, textSize);
        //     elemPos += increment;

        //     auto node = credits->AddChild<Canvas>(rowSize);
        //     node->SetLocation(elemPos);
        //     elemPos += increment;

        //     auto name1 = node->AddChild<UITextElement>(font, "Name 3", glm::vec2(), textSize);
        //     name1->anchorPoint = UIElement::AnchorPoint::eTopLeft;

        //     auto name2 = node->AddChild<UITextElement>(font, "Name 4", glm::vec2(), textSize);
        //     name2->anchorPoint = UIElement::AnchorPoint::eTopRight;
        // }

        // // Production
        // {
        //     credits->AddChild<UITextElement>(font, "Graphics", elemPos, textSize);
        //     elemPos += increment;

        //     auto name1 = credits->AddChild<UITextElement>(font, "Name 5", elemPos, textSize);
        //     name1->anchorPoint = UIElement::AnchorPoint::eMiddle;
        // }

        // BACK BUTTON
        {
            constexpr glm::vec2 backPos = { 0.0f, 525.0f };
            auto back = credits->AddChild<UIButton>(buttonStyle, backPos, buttonBaseSize);
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
            credits->backButton = back;
        }
    }

    credits->UpdateAllChildrenAbsoluteTransform();
    graphicsContext.UpdateBindlessSet();

    return credits;
}
