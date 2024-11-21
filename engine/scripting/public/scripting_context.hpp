#pragma once
#include <common.hpp>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct WrenVM;

class ScriptingContext
{
public:
    // These default values are the same as specified in wren.h
    struct VMMemoryConfig
    {
        size_t initialHeapSize = 1024ull * 1024ull * 10ull; // 10 MiB
        size_t minHeapSize = 1024ull * 1024ull; // 1 MiB
        uint32_t heapGrowthPercent = 50;
    };

    ScriptingContext(const VMMemoryConfig& memory_config);
    ~ScriptingContext();

    size_t GetModuleCount() const { return _loadedModulePaths.size(); }

    bool InterpretWrenModule(const std::string& path);

    // Sets the output stream for system log calls
    void SetScriptingOutputStream(std::ostream* stream) { _wrenOutStream = stream; }

    // Adds an include path to wren scripts
    // Will not add another entry if it maps to the same directory
    void AddWrenIncludePath(const std::string& path);

    NON_COPYABLE(ScriptingContext);
    NON_MOVABLE(ScriptingContext);

private:
    WrenVM* _vm;
    std::ostream* _wrenOutStream = &std::cout;

    // Contains Wren callbacks, used by the VM
    // Needs friend access to work with the internals of the scripting context
    friend struct WrenCallbacks;

    std::optional<std::string> LoadModuleSource(const std::string& modulePath);

    std::vector<std::string> _loadedModulePaths {};
    std::vector<std::string> _includePaths {};
};