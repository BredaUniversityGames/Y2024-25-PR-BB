#include <thread_pool.hpp>

ThreadPool::ThreadPool(uint32_t threadCount)
{
    for (uint32_t i = 0; i < threadCount; i++)
    {
        _threads.emplace_back(std::thread(WorkerMain, this));
    }
}

ThreadPool::~ThreadPool()
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

void ThreadPool::Start()
{
    {
        std::scoped_lock<std::mutex> lock { _mutex };
        _running = true;
    }
    _workerNotify.notify_all();
}

void ThreadPool::CancelAll()
{
    {
        std::scoped_lock<std::mutex> lock { _mutex };
        _tasks = {};
        _running = false;
    }
    _workerNotify.notify_all();
}

void ThreadPool::FinishPendingWork()
{
    if (!_running)
        return;

    std::unique_lock<std::mutex> lock { _mutex };
    _ownerNotify.wait(lock, [this]()
        { return _tasks.empty(); });
}

void ThreadPool::WorkerMain(ThreadPool* pool)
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