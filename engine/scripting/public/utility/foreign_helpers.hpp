#pragma once

#include <string>

namespace ScriptingUtility
{
std::string MakeForeignClassName(const char* module, const char* name);
std::string MakeForeignMethodName(const char* module, const char* className, const char* methodName, bool isStatic);
}