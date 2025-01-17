#include "settings.hpp"

#include <filesystem>
#include <fstream>

#include "cereal/archives/json.hpp"
#include "cereal/cereal.hpp"

SettingsStore& SettingsStore::Instance()
{
    static SettingsStore instance {};

    return instance;
}

void SettingsStore::Write()
{
    std::ofstream stream { "settings.json" };

    if (stream)
    {
        cereal::JSONOutputArchive archive { stream };
        archive(cereal::make_nvp("settings", settings));
    }
}

SettingsStore::SettingsStore()
{
    std::ifstream stream { "settings.json" };

    if (stream)
    {
        cereal::JSONInputArchive archive { stream };
        archive(cereal::make_nvp("settings", settings));
    }
}
