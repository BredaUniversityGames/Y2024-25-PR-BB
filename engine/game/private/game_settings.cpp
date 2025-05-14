#include "game_settings.hpp"
#include "file_io.hpp"
#include <cereal/archives/json.hpp>

GameSettings GameSettings::FromFile(const std::string& path)
{
    GameSettings out {};
    if (auto stream = fileIO::OpenReadStream(path))
    {
        cereal::JSONInputArchive ar { stream.value() };
        ar(out);
    }
    else
    {
        out.SaveToFile(path);
    }
    return out;
}
void GameSettings::SaveToFile(const std::string& path) const
{
    if (auto stream = fileIO::OpenWriteStream(path))
    {
        cereal::JSONOutputArchive ar { stream.value() };
        ar(*this);
    }
    else
    {
        bblog::error("Error serializing user settings: {}", path);
    }
}
