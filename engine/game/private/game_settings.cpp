#include "game_settings.hpp"
#include "file_io.hpp"
#include <cereal/archives/json.hpp>

GameSettings GameSettings::FromFile(const std::string& path)
{
    GameSettings out {};
    if (auto stream = std::ifstream { path })
    {
        cereal::JSONInputArchive ar { stream };

        try
        {
            ar(out);
        }
        catch (const std::exception& e)
        {
            bblog::warn("Outdated settings file, reverting to defaults.");
            out.SaveToFile(path);
        }
    }
    else
    {
        out.SaveToFile(path);
    }
    return out;
}
void GameSettings::SaveToFile(const std::string& path) const
{
    if (auto stream = std::ofstream { path })
    {
        cereal::JSONOutputArchive ar { stream };
        ar(*this);
    }
    else
    {
        bblog::error("Error serializing user settings: {}", path);
    }
}
