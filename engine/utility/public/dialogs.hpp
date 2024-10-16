#pragma once
#include <filesystem>
#include <string.h>
#include <vector>

class Filters
{
public:
    Filters()
        : _val("All Files (*.*)\0*.*\0")
    {
    }
    Filters(const std::vector<std::pair<const char*, const char*>>& filters)
    {
        std::vector<char> filterVec;
        for (const auto& filter : filters)
        {
            // Append the display string
            filterVec.insert(filterVec.end(), filter.first, filter.first + strlen(filter.first));
            filterVec.push_back('\0');
            // Append the filter pattern
            filterVec.insert(filterVec.end(), filter.second, filter.second + strlen(filter.second));
            filterVec.push_back('\0');
        }
        filterVec.push_back('\0'); // Double null-terminate
        _val = std::string(filterVec.data());
    }

    [[nodiscard]] std::string_view Value() const noexcept
    {
        return _val;
    }

private:
    std::string _val {};
};
static std::filesystem::path OpenFileDialog(const Filters& filters = Filters());
