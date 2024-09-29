#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool
{
public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    void enqueue(std::function<void()> task);
    void exit(); // הוסף את הפונקציה stop() כ-public

private:
    // חוטי עבודה
    std::vector<std::thread> workers;
    // תור של משימות
    std::queue<std::function<void()>> tasks;
    // סנכרון
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

    void workerThread();
};