#pragma once

#include <filesystem>
#include <fstream>

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"
#include "log.hpp"

namespace Serialization
{
/**
 * Serializes an object to JSON, calls the save function of the object.
 * @tparam Object Object type, deduced by function argument.
 * @param path File to write to.
 * @param object Object to serialize.
 */
template <typename T>
void SerialiseToJSON(const std::filesystem::path& path, const T& object)
{
    std::ofstream stream { path };
    if (stream)
    {
        cereal::JSONOutputArchive archive { stream };
        archive(cereal::make_nvp(typeid(T).name(), object));
    }
    else
    {
        bblog::error("Failed to write to file in Serialization::SerialiseToJson with filepath {}", path.string());
    }
}
}
