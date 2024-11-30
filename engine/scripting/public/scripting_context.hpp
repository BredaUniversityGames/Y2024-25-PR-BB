#pragma once

#include "common.hpp"
#include "utility/foreign_bindings.hpp"
#include "wren_common.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// These default values are the same as specified in wren.h
struct ScriptingInitConfig
{
    std::vector<std::string> includePaths;
    size_t initialHeapSize = 1024ull * 1024ull * 10ull; // 10 MiB
    size_t minHeapSize = 1024ull * 1024ull; // 1 MiB
    uint32_t heapGrowthPercent = 50;
};

class ScriptingContext
{
public:
    ScriptingContext(const ScriptingInitConfig& info);
    ~ScriptingContext();

    NON_COPYABLE(ScriptingContext);
    NON_MOVABLE(ScriptingContext);

    NO_DISCARD wren::VM& GetVM() { return *_vm; }

    // Interpret a Wren Script, returns the name identifier of the script on success
    std::optional<std::string> RunScript(const std::string& path);

    // Sets the output stream for system log calls
    void SetScriptingOutputStream(std::ostream* stream) { _wrenOutStream = stream; }
    void FlushOutputStream() { _wrenOutStream->flush(); }

private:
    std::unique_ptr<wren::VM> _vm;
    std::ostream* _wrenOutStream = &std::cout;
};