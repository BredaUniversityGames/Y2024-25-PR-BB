#pragma once
#include "module_interface.hpp"
#include "thread_pool.hpp"

class ThreadModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {

        return ModuleTickOrder::eTick; // Module doesn't tick
    }

    void Tick(MAYBE_UNUSED Engine& engine) override {};
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};

    std::string_view GetName() override { return "Thread Module"; }

    // ThreadPool _thread_pool {};

public:
    NON_COPYABLE(ThreadModule);
    NON_MOVABLE(ThreadModule);

    ThreadModule() = default;
    ~ThreadModule() override = default;
};