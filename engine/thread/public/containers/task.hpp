#pragma once
#include "common.hpp"
#include <future>

namespace detail
{

class CallableInterface
{
public:
    CallableInterface() = default;
    virtual ~CallableInterface() = default;
    virtual void Call() = 0;

    NON_MOVABLE(CallableInterface);
    NON_COPYABLE(CallableInterface);
};

template <typename Ret>
class BasicTaskImpl : public CallableInterface
{
public:
    BasicTaskImpl(std::packaged_task<Ret()>&& task)
        : _task(std::move(task)) {};
    virtual ~BasicTaskImpl() = default;

    void Call() override { _task(); };

    NON_MOVABLE(BasicTaskImpl);
    NON_COPYABLE(BasicTaskImpl);

private:
    std::packaged_task<Ret()> _task;
};

}

class Task
{
public:
    Task() = default;
    Task(std::unique_ptr<detail::CallableInterface>&& callable)
        : _callable(std::move(callable))
    {
    }
    void Run() { _callable->Call(); }
    bool Valid() const { return _callable != nullptr; }

private:
    std::unique_ptr<detail::CallableInterface> _callable {};
};