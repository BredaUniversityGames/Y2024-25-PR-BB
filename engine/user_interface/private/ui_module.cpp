#include "ui_module.hpp"
#include "canvas.hpp"
#include <application_module.hpp>
#include <renderer.hpp>
#include <renderer_module.hpp>

ModuleTickOrder UIModule::Init(Engine& engine)
{
    const glm::vec2 extend = engine.GetModule<ApplicationModule>().DisplaySize();
    _viewport = std::make_unique<Viewport>(extend);
    _graphicsContext = engine.GetModule<RendererModule>().GetGraphicsContext();
    return ModuleTickOrder::ePostTick;
}

void UIModule::Tick(Engine& engine)
{
    _uiInputContext._gamepadHasFocus = engine.GetModule<ApplicationModule>().GetInputDeviceManager().IsGamepadAvailable();

    InputManagers inputManagers = {
        .actionManager = engine.GetModule<ApplicationModule>().GetActionManager(),
        .inputDeviceManager = engine.GetModule<ApplicationModule>().GetInputDeviceManager()
    };

    _viewport->Update(inputManagers, _uiInputContext);
    _uiInputContext._hasInputBeenConsumed = false;
}

UINavigationMappings::Direction UIInputContext::GetDirection(const ActionManager& actionManager)
{
    glm::vec2 actionValue = actionManager.GetAnalogAction(_navigationActionName);
    glm::vec2 absActionValue = glm::abs(actionValue);

    UINavigationMappings::Direction direction = UINavigationMappings::Direction::eNone;
    if (absActionValue.x > absActionValue.y && actionValue.x > 0.1f)
    {
        direction = UINavigationMappings::Direction::eRight;
    }
    else if (absActionValue.x > absActionValue.y && actionValue.x < -0.1f)
    {
        direction = UINavigationMappings::Direction::eLeft;
    }
    else if (absActionValue.y > absActionValue.x && actionValue.y > 0.1f)
    {
        direction = UINavigationMappings::Direction::eUp;
    }
    else if (absActionValue.y > absActionValue.x && actionValue.y < -0.1f)
    {
        direction = UINavigationMappings::Direction::eDown;
    }

    if (direction == _previousNavigationDirection)
    {
        return UINavigationMappings::Direction::eNone;
    }

    _previousNavigationDirection = direction;
    return direction;
}
void UIModule::CreateMainMenu(std::function<void()> onPlayButtonClick, std::function<void()> onExitButtonClick)
{
    GetViewport().AddElement<Canvas>(CreateMainMenuCanvas(_uiInputContext, _viewport->GetExtend(), _graphicsContext, onPlayButtonClick, onExitButtonClick));
}
