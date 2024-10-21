#pragma once

#include <filesystem>
#include <fstream>

#include "cereal/cereal.hpp"
#include "cereal/archives/json.hpp"

namespace Serialization
{
/**
 * Serializes an object to JSON, calls the save function of the object.
 * @tparam Object Object type, deduced by function argument.
 * @param path File to write to.
 * @param object Object to serialize.
 */
template <typename Object>
void SerialiseToJSON(const std::filesystem::path& path, const Object& object)
{
    std::ofstream os;
    os.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    os.open(path);

    cereal::JSONOutputArchive ar(os);
    ar(cereal::make_nvp(typeid(Object).name(), object));
}
}
