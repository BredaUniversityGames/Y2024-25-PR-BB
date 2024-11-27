#pragma once

#include "common.hpp"
#include "utility/foreign_bindings.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <wren_common.hpp>

class ScriptingContext
{
public:
    // These default values are the same as specified in wren.h
    struct VMInitConfig
    {
        std::vector<std::string> includePaths;
        size_t initialHeapSize = 1024ull * 1024ull * 10ull; // 10 MiB
        size_t minHeapSize = 1024ull * 1024ull; // 1 MiB
        uint32_t heapGrowthPercent = 50;
    };

    ScriptingContext(const VMInitConfig& info);
    ~ScriptingContext();

    wren::VM& GetVM() { return *_vm; }

    NON_COPYABLE(ScriptingContext);
    NON_MOVABLE(ScriptingContext);

    // Interpret a Wren Module, returns the module name on success
    std::optional<std::string> InterpretWrenModule(const std::string& path);

    // Sets the output stream for system log calls
    void SetScriptingOutputStream(std::ostream* stream) { _wrenOutStream = stream; }
    void FlushOutputStream() { _wrenOutStream->flush(); }

private:
    std::unique_ptr<wren::VM> _vm;
    std::ostream* _wrenOutStream = &std::cout;

    // std::optional<std::string> LoadModuleSource(const std::string& modulePath);
    //
    // std::vector<std::string> _loadedModulePaths {};
    // std::vector<std::string> _includePaths {};
    //
    // // Foreign storage
    //
    // std::unordered_map<std::string, WrenForeignClassMethods> _classes {};
    // std::unordered_map<std::string, WrenForeignMethodFn> _methods {};
};