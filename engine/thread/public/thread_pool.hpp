#pragma once
#include "containers/task.hpp"

#include <queue>
#include <thread>

class ThreadPool
{
public:
    ThreadPool(uint32_t threadCount)
    {
        for (uint32_t i = 0; i < threadCount; i++)
        {
            _threads.emplace_back(std::thread(WorkerMain, this));
        }
    }

    ~ThreadPool()
    {
        {
            std::scoped_lock<std::mutex> lock { _mutex };
            _kill = true;
        }

        _workerNotify.notify_all();

        for (auto& thread : _threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    template <typename Ret>
    std::future<Ret> QueueWork(std::packaged_task<Ret()>&& task)
    {
        auto future = task.get_future();
        {
            std::scoped_lock<std::mutex> lock { _mutex };
            _tasks.emplace(std::make_unique<detail::BasicTaskImpl<Ret>>(std::move(task)));
        }
        return future;
    }

    // Startes the thread pool, making workers continuosly consume work until FinishWork() or Cancel() is called
    void Start()
    {
        {
            std::scoped_lock<std::mutex> lock { _mutex };
            _running = true;
        }
        _workerNotify.notify_all();
    }

    // Stops the threadpool from running any additional jobs and clears the queue beyond the ones that are already running
    // Start() must be called again to queue and run any jobs added afterwards
    void CancelAll()
    {
        {
            std::scoped_lock<std::mutex> lock { _mutex };
            _tasks = {};
            _running = false;
        }
        _workerNotify.notify_all();
    }

    // Blocks the calling thread until all queued work is done
    // Start() must be called again to queue and run any jobs added afterwards
    void FinishWork()
    {
        if (!_running)
            return;

        std::unique_lock<std::mutex> lock { _mutex };
        _ownerNotify.wait(lock, [this]()
            { return _tasks.empty(); });
    }

private:
    static void WorkerMain(ThreadPool* pool)
    {
        while (true)
        {
            Task my_task {};

            {
                // wait for a notify to start or kill the thread

                std::unique_lock<std::mutex> lock(pool->_mutex);
                pool->_workerNotify.wait(lock, [pool]()
                    { return pool->_kill == true || (pool->_tasks.size() > 0 && pool->_running == true); });

                // After a wait, the lock is acquired

                if (pool->_kill == true)
                {
                    return;
                }

                my_task = std::move(pool->_tasks.front());
                pool->_tasks.pop();
            }

            my_task.Run();
            pool->_ownerNotify.notify_all();
        }
    }

    std::mutex _mutex;
    std::condition_variable _workerNotify;
    std::condition_variable _ownerNotify;
    std::vector<std::thread> _threads {};
    std::queue<Task> _tasks;
    bool _running = false;
    bool _kill = false;
};