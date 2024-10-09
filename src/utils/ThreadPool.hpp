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
    void exit();

private:
    // Worker threads
    std::vector<std::thread> workers;
    // Task queue
    std::queue<std::function<void()>> tasks;
    // Synchronization
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;

    // Leader-Follower pattern additions
    std::atomic<std::thread::id> currentLeader;
    std::condition_variable leaderCondition;
    std::mutex leaderMutex;

    void workerThread();
    void becomeLeader();
    void promoteNewLeader();
};