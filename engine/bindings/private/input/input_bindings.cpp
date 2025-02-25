#include "input_bindings.hpp"
#include "application_module.hpp"
#include "input/action_manager.hpp"
#include "input/input_codes/keys.hpp"
#include "utility/enum_bind.hpp"

namespace bindings
{
DigitalActionResult GetDigitalAction(ApplicationModule& self, const std::string& action_name)
{
    return self.GetActionManager().GetDigitalAction(action_name);
}

bool GetDigitalActionIsPressed(DigitalActionResult& self)
{
    return self.IsPressed();
}

bool GetDigitalActionIsHeld(DigitalActionResult& self)
{
    return self.IsHeld();
}

bool GetDigitalActionIsReleased(DigitalActionResult& self)
{
    return self.IsReleased();
}

glm::vec2 GetAnalogAction(ApplicationModule& self, const std::string& action_name)
{
    return self.GetActionManager().GetAnalogAction(action_name);
}

glm::vec2 GetMousePosition(ApplicationModule& self)
{
    int32_t mouseX, mouseY;
    self.GetInputDeviceManager().GetMousePosition(mouseX, mouseY);
    return glm::vec2(mouseX, mouseY);
}

bool IsInputEnabled(ApplicationModule& self)
{
    return self.GetMouseHidden();
}

bool GetRawKeyOnce(ApplicationModule& self, KeyboardCode code)
{
    return self.GetInputDeviceManager().IsKeyPressed(code);
}

bool GetRawKeyHeld(ApplicationModule& self, KeyboardCode code)
{
    return self.GetInputDeviceManager().IsKeyHeld(code);
}
}

void BindInputAPI(wren::ForeignModule& module)
{
    auto& wrenClass = module.klass<ApplicationModule>("Input");
    wrenClass.funcExt<bindings::GetDigitalAction>("GetDigitalAction");
    wrenClass.funcExt<bindings::GetAnalogAction>("GetAnalogAction");
    wrenClass.funcExt<bindings::GetRawKeyOnce>("DebugGetKey");
    wrenClass.funcExt<bindings::GetRawKeyHeld>("DebugGetHeldKey");
    wrenClass.funcExt<bindings::GetMousePosition>("GetMousePosition");
    wrenClass.funcExt<bindings::IsInputEnabled>("DebugIsInputEnabled");

    auto& digitalActionResult = module.klass<DigitalActionResult>("DigitalActionResult");
    digitalActionResult.funcExt<bindings::GetDigitalActionIsPressed>("IsPressed");
    digitalActionResult.funcExt<bindings::GetDigitalActionIsHeld>("IsHeld");
    digitalActionResult.funcExt<bindings::GetDigitalActionIsReleased>("IsReleased");

    bindings::BindEnum<KeyboardCode>(module, "Keycode");
}