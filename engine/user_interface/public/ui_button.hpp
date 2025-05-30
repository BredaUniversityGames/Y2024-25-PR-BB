#pragma once
#include "callback.hpp"
#include "resource_manager.hpp"
#include "resources/image.hpp"
#include "ui_element.hpp"

#include <functional>
#include <cereal/types/base_class.hpp>

class UIButton : public UIElement
{
public:
    UIButton() : UIElement()
    {
    }

    enum class ButtonState
    {
        eNormal,
        eHovered,
        ePressed
    };

    void Update(const InputManagers& inputManagers, UIInputContext& inputContext) override;

    // ButtonState GetState() const { return state; }
    // bool IsPressedOnce() const { return state == ButtonState::ePressed && previousState != ButtonState::ePressed; }

    struct ButtonStyle
    {
        ResourceHandle<GPUImage> normalImage = {};
        ResourceHandle<GPUImage> hoveredImage = {};
        ResourceHandle<GPUImage> pressedImage = {};
    } style {};

    UIButton(ButtonStyle aStyle, const glm::vec2& location, const glm::vec2& size)
        : style(aStyle)
    {
        SetLocation(location);
        SetScale(size);
    }

    void SubmitDrawInfo(std::vector<QuadDrawInfo>& drawList) const override;
    void OnPress(Callback callback) { _callback = callback; }

private:
    void SwitchState(bool inputActionPressed, bool inputActionReleased);

    ButtonState state = ButtonState::eNormal;
    Callback _callback {};
};

template <class Archive>
void save(Archive& ar,const UIButton::ButtonStyle& style)
{
    
    
    ar(CEREAL_NVP(style.normalImage));
    ar(CEREAL_NVP(style.hoveredImage));
    ar(CEREAL_NVP(style.pressedImage));
}


template <class Archive>
void load(Archive& ar,UIButton::ButtonStyle& style)
{
    ar(CEREAL_NVP(style.normalImage));
    ar(CEREAL_NVP(style.hoveredImage));
    ar(CEREAL_NVP(style.pressedImage));
}


template <class Archive>
void save(Archive& ar,const UIButton& button)
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&button)));
    ar(cereal::make_nvp("style", button.style));
}

template <class Archive>
void load(Archive& ar, UIButton& button)
{
    ar(cereal::make_nvp("base", cereal::base_class<UIElement>(&button)));
    ar(cereal::make_nvp("style", button.style));
    
}



CEREAL_REGISTER_TYPE(UIButton);
