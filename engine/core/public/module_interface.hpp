#pragma once
#include "fwd_core.hpp"
#include <cstdint>

namespace ModuleTickOrder
{

enum Enum : uint32_t
{
    eFirst = 0,
    ePreTick = 5,
    eTick = 10,
    ePostTick = 15,
    ePreRender = 20,
    eRender = 25,
    ePostRender = 30,
    eLast = 35
};

}

// Main interface for defining engine modules
// Requires overriding: Init, Tick, Shutdown
class ModuleInterface
{
public:
    virtual ~ModuleInterface() = default;

private:
    friend Engine;
    friend MainEngine;

    // Return the desired tick order for this module
    virtual uint32_t Init(Engine& engine) = 0;

    // Ticking order is decided based on the returned value from Init
    virtual void Tick(Engine& engine) = 0;

    // Modules are shutdown in the order they are initialized
    virtual void Shutdown(Engine& engine) = 0;
};