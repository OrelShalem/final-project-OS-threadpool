#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t threads) : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
    {
        workers.emplace_back([this]()
                             { workerThread(); });
    }
}

ThreadPool::~ThreadPool()
{
    exit();
}

void ThreadPool::enqueue(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(queueMutex);
    tasks.push(std::move(task));
    condition.notify_one();
}

void ThreadPool::workerThread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]
                           { return stop || !tasks.empty(); });
            if (stop && tasks.empty())
                return;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

void ThreadPool::exit()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
        condition.notify_all();
    }
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            try
            {
                worker.join();
            }
            catch (const std::system_error &e)
            {
                // התעלם משגיאות בזמן סגירת החוטים
            }
        }
    }
}