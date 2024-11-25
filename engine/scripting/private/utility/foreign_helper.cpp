#include "utility/foreign_helpers.hpp"

std::string ScriptingUtility::MakeForeignClassName(const char* module, const char* name)
{
    return std::string(module) + "::" + name;
}

std::string ScriptingUtility::MakeForeignMethodName(const char* module, const char* className, const char* methodName, bool isStatic)
{
    const char* separator = isStatic ? "::" : ".";
    return std::string(module) + "::" + className + separator + methodName;
}