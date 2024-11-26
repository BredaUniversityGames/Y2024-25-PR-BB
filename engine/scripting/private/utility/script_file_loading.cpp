#include "utility/script_file_loading.hpp"
#include "wren_common.hpp"

#include <file_io.hpp>
#include <filesystem>

using FilePath = std::filesystem::path;

std::string ScriptFileLoading::CanonicalizePath(const std::string& path)
{
    return FilePath(path).make_preferred().string();
}

std::string ScriptFileLoading::LoadModuleFromFile(const std::vector<std::string>& includePaths, const std::string& modulePath)
{
    if (fileIO::Exists(modulePath))
    {
        if (auto stream = fileIO::OpenReadStream(modulePath, fileIO::DEFAULT_READ_FLAGS))
        {
            return fileIO::DumpStreamIntoString(stream.value());
        }
        throw wren::Exception("Failed to read module " + modulePath);
    }

    for (auto& includePath : includePaths)
    {
        std::string tryPath = includePath + modulePath;
        if (fileIO::Exists(tryPath))
        {
            if (auto stream = fileIO::OpenReadStream(tryPath, fileIO::DEFAULT_READ_FLAGS))
            {
                return fileIO::DumpStreamIntoString(stream.value());
            }
            throw wren::Exception("Failed to read module " + modulePath);
        }
    }

    throw wren::Exception("Could not find module named " + modulePath);
}