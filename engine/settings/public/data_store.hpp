#pragma once

#undef Bool
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <filesystem>
#include <fstream>

#include "file_io.hpp"

template <class T>
class DataStore
{
public:
    DataStore(const std::filesystem::path& path)
        : _path(path)
    {
        if (auto stream = fileIO::OpenReadStream(path.generic_string()))
        {
            cereal::JSONInputArchive archive { stream.value() };
            archive(cereal::make_nvp("data", data));
        }
    }

    void Write()
    {
        if (auto stream = fileIO::OpenWriteStream(_path.generic_string()))
        {
            cereal::JSONOutputArchive archive { stream.value() };
            archive(cereal::make_nvp("data", data));
        }
    }

    ~DataStore() = default;

    T data;

private:
    std::filesystem::path _path;
};
