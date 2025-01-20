#include "game_module.hpp"
#include "ecs_module.hpp"
#include "lifetime_system.hpp"
#include "renderer_module.hpp"
#include "time_module.hpp"
#include "ui_menus.hpp"
#include "ui_module.hpp"

ModuleTickOrder GameModule::Init(Engine& engine)
{
    auto& ECS = engine.GetModule<ECSModule>();
    ECS.AddSystem<LifetimeSystem>();

    auto hud = HudCreate(*engine.GetModule<RendererModule>().GetGraphicsContext(), engine.GetModule<UIModule>().GetViewport().GetExtend());
    _hud = hud.second;
    engine.GetModule<UIModule>().GetViewport().AddElement<Canvas>(std::move(hud.first));

    return ModuleTickOrder::eTick;
}
void GameModule::Shutdown(MAYBE_UNUSED Engine& engine)
{
}
void GameModule::Tick(MAYBE_UNUSED Engine& engine)
{
    if (_updateHud == true)
    {
        float totalTime = engine.GetModule<TimeModule>().GetTotalTime().count();
        HudUpdate(_hud, totalTime);
    }
}