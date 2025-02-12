#pragma once
#include "containers/task.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

class ThreadPool
{
public:
    ThreadPool(uint32_t threadCount);
    ~ThreadPool();

    template <typename Functor, typename... Args>
    auto QueueWork(Functor&& f)
    {
        using Ret = std::invoke_result_t<Functor>;
        auto packaged = std::packaged_task<Ret()>(std::move(f));
        auto future = packaged.get_future();

        {
            std::scoped_lock<std::mutex> lock { _mutex };
            _tasks.emplace(std::make_unique<detail::BasicTaskImpl<Ret>>(std::move(packaged)));
        }

        _workerNotify.notify_one();
        return future;
    }

    // Starts the thread pool, making workers continuously consume work until FinishWork() or Cancel() is called
    void Start();

    // Stops the threadpool from running any additional jobs and clears the queue beyond the ones that are already running
    // Start() must be called again to queue and run any jobs added afterwards
    void CancelAll();

    // Blocks the calling thread until all work in the queue is complete
    // WARN: any futures that are cancelled will throw if accessed
    void FinishPendingWork();

private:
    static void WorkerMain(ThreadPool* pool, uint32_t ID);

    std::mutex _mutex;
    std::condition_variable _workerNotify;
    std::condition_variable _ownerNotify;
    std::vector<std::thread> _threads {};
    std::queue<Task> _tasks;
    bool _running = false;
    bool _kill = false;
};