#include "steam_bindings.hpp"

#include "achievements.hpp"
#include "game_module.hpp"
#include "steam_module.hpp"
#include "steam_stats.hpp"
#include "utility/enum_bind.hpp"

#include <magic_enum.hpp>

namespace bindings
{
Stat* GetStat(SteamModule& self, SteamStatEnum stats)
{
    return self.GetStats().GetStat(magic_enum::enum_name(stats));
}
Achievement* GetAchievement(SteamModule& self, SteamAchievementEnum achievements)
{
    return self.GetAchievements().GetAchievement(magic_enum::enum_name(achievements));
}

std::string_view GetAchievementName(Achievement& achievement)
{
    return std::string_view(achievement.name);
}
std::string_view GetAchievementDescription(Achievement& achievement)
{
    return std::string_view(achievement.description);
}

void Unlock(SteamModule& self, SteamAchievementEnum achievements)
{
    self.GetAchievements().SetAchievement(magic_enum::enum_name(achievements).data());
}

}

void BindSteamAPI(wren::ForeignModule& module)
{
    auto& steamClass = module.klass<SteamModule>("Steam");
    steamClass.funcExt<bindings::GetAchievement>("GetAchievement");
    steamClass.funcExt<bindings::GetStat>("GetStat");
    steamClass.funcExt<bindings::Unlock>("Unlock");

    auto& statClass = module.klass<Stat>("Stat");
    statClass.var<&Stat::value>("intValue");
    statClass.var<&Stat::floatValue>("floatValue");
    statClass.varReadonly<&Stat::name>("name");

    auto& achievementClass = module.klass<Achievement>("Achievement");
    achievementClass.varReadonly<&Achievement::achieved>("achieved");
    achievementClass.propReadonlyExt<bindings::GetAchievementDescription>("description");
    achievementClass.propReadonlyExt<bindings::GetAchievementName>("name");

    bindings::BindEnum<SteamStatEnum>(module, "Stats");
    bindings::BindEnum<SteamAchievementEnum>(module, "Achievements");
}
