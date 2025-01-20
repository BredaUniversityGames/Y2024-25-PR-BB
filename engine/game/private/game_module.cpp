#include "game_module.hpp"

#include "ecs_module.hpp"
#include "lifetime_system.hpp"

#include "ui_menus.hpp"
#include "ui_module.hpp"

#include <renderer_module.hpp>
#include <time_module.hpp>
#include <ui_progress_bar.hpp>
#include <ui_text.hpp>

ModuleTickOrder GameModule::Init(Engine& engine)
{
    auto& ECS = engine.GetModule<ECSModule>();
    ECS.AddSystem<LifetimeSystem>();

    auto hud = CreateHud(*engine.GetModule<RendererModule>().GetGraphicsContext(), engine.GetModule<UIModule>().GetViewport().GetExtend());
    _hud = hud.second;
    engine.GetModule<UIModule>().GetViewport().AddElement<Canvas>(std::move(hud.first));

    return ModuleTickOrder::eTick;
}
void GameModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
}
void GameModule::Tick(MAYBE_UNUSED Engine& engine)
{

    // all temporary

    float fract = (sin(engine.GetModule<TimeModule>().GetTotalTime().count() / 100.0f) + 1) * 0.5;
    if (auto locked = _hud.healthBar.lock(); locked != nullptr)
    {
        locked->SetPercentage(fract);
    }

    if (auto locked = _hud.ultBar.lock(); locked != nullptr)
    {
        locked->SetPercentage(fract);
    }

    if (auto locked = _hud.sprintBar.lock(); locked != nullptr)
    {
        locked->SetPercentage(fract);
    }

    if (auto locked = _hud.grenadeBar.lock(); locked != nullptr)
    {
        locked->SetPercentage(fract);
    }

    if (auto locked = _hud.ammoCounter.lock(); locked != nullptr)
    {
        static float ammo = 8.0f;
        ammo -= 0.01;
        if (ammo <= 0.0f)
        {
            ammo = 8.0f;
        }

        locked->SetText(std::to_string(int(ammo)) + "/8");
    }
}