#pragma once
#include "module_interface.hpp"
#include "thread_pool.hpp"

class ThreadModule : public ModuleInterface
{
    ModuleTickOrder Init(MAYBE_UNUSED Engine& engine) override
    {
        _threadPool = std::make_unique<ThreadPool>(std::thread::hardware_concurrency());
        _threadPool->Start();

        return ModuleTickOrder::eTick; // Module doesn't tick
    }

    void Tick(MAYBE_UNUSED Engine& engine) override {};
    void Shutdown(MAYBE_UNUSED Engine& engine) override {};
    std::string_view GetName() override { return "Thread Module"; }

    std::unique_ptr<ThreadPool> _threadPool;

public:
    NON_COPYABLE(ThreadModule);
    NON_MOVABLE(ThreadModule);

    ThreadModule() = default;
    ~ThreadModule() override = default;

    ThreadPool& GetPool() { return *_threadPool; }
};