#pragma once
#include "common.hpp"

#include "wren_common.hpp"

namespace ScriptFileLoading
{

std::string CanonicalizePath(const std::string& path);
std::string LoadModuleFromFile(const std::vector<std::string>& includePaths, const std::string& modulePath);

}