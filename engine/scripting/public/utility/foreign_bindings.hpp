#pragma once
#include "engine.hpp"
#include "wren_common.hpp"

#include <ostream>

struct ForeignBindings
{
    ForeignBindings(const std::string& engineModule);

    std::string GetModuleName() const { return _engineModule; }
    wren::ForeignModule& GetForeignModule(wren::VM& vm) const { return vm.module(_engineModule); };

    void WriteToStream(wren::VM& vm, std::ostream& output) const;

private:
    std::string _engineModule;
};
