#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace text
{

inline std::optional<std::string_view> NameFromFilePath(std::string_view path)
{
    int start = path.find_last_of('/');
    int end = path.find_last_of('.');

    if (start != std::string::npos && end != std::string::npos)
    {
        return path.substr(start + 1, end - start);
    }

    return std::nullopt;
}

inline std::optional<std::string_view> DirectoryFromFilePath(std::string_view path)
{
    int end = path.find_last_of('/');

    if (end != std::string::npos)
    {
        return path.substr(0, end);
    }

    return std::nullopt;
}

}