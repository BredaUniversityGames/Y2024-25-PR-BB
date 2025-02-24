#pragma once

#undef Bool
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <filesystem>
#include <fstream>

template <class T>
class DataStore
{
public:
    DataStore(const std::filesystem::path& path)
        : _path(path)
    {
        std::ifstream stream { path.c_str() };

        if (stream)
        {
            cereal::JSONInputArchive archive { stream };
            archive(cereal::make_nvp("data", data));
        }
    }

    void Write()
    {
        std::ofstream stream { "settings.json" };

        if (stream)
        {
            cereal::JSONOutputArchive archive { stream };
            archive(cereal::make_nvp("data", data));
        }
    }

    ~DataStore()
    {
        Write();
    }

    T data;

private:
    std::filesystem::path _path;
};
