#include "ThreadPool.hpp"

// Constructor: Initializes the thread pool with a specified number of threads
ThreadPool::ThreadPool(size_t threads) : stop(false), currentLeader(std::thread::id())
{
    for (size_t i = 0; i < threads; ++i)
    {
        // Create a new worker thread and add it to the workers vector
        // The thread executes the workerThread function of the current ThreadPool instance
        workers.emplace_back([this]()
                             { workerThread(); });
    }
}

// Destructor: Ensures all threads are properly stopped and joined
ThreadPool::~ThreadPool()
{
    exit();
}

// Adds a new task to the queue
void ThreadPool::enqueue(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(queueMutex);
    tasks.push(std::move(task));
    condition.notify_one();
}

// Main worker thread function
void ThreadPool::workerThread()
{
    while (true)
    {
        becomeLeader(); // Leader-Follower pattern: Thread becomes the leader

        std::function<void()> task;
        {
            // Acquire a lock on the queue mutex to ensure thread-safe access to the task queue
            std::unique_lock<std::mutex> lock(queueMutex);

            // Wait until either the stop flag is set or there are tasks in the queue
            // This prevents busy-waiting and allows the thread to sleep when there's no work
            condition.wait(lock, [this]
                           { 
                               // The lambda function returns true if the thread should wake up:
                               // - If 'stop' is true, indicating the thread pool is shutting down
                               // - Or if the task queue is not empty, meaning there's work to do
                               return stop || !tasks.empty(); });

            if (stop && tasks.empty())
            {
                promoteNewLeader(); // Leader-Follower pattern: Promote a new leader before exiting
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }

        promoteNewLeader(); // Leader-Follower pattern: Promote a new leader before executing the task
        task();
    }
}

// Leader-Follower pattern: Thread becomes the leader
void ThreadPool::becomeLeader()
{
    // Acquire a unique lock on the leader mutex to ensure thread-safe access
    std::unique_lock<std::mutex> leaderLock(leaderMutex);

    // Wait until there's no current leader or this thread is already the leader
    leaderCondition.wait(leaderLock, [this]
                         { 
                             // Check if there's no current leader (default thread ID)
                             // or if this thread is already the leader
                             return currentLeader.load() == std::thread::id() || 
                                    currentLeader.load() == std::this_thread::get_id(); });

    // Set this thread as the current leader
    currentLeader.store(std::this_thread::get_id());
}

// Leader-Follower pattern: Promotes a new leader
void ThreadPool::promoteNewLeader()
{
    // This function is responsible for promoting a new leader in the Leader-Follower pattern

    // Create a lock guard to ensure thread-safe access to the leader-related data
    std::lock_guard<std::mutex> leaderLock(leaderMutex);

    // Reset the current leader to a default thread ID
    // This indicates that there is no active leader at the moment
    currentLeader.store(std::thread::id());

    // Notify one waiting thread that it can become the new leader
    // This wakes up a thread that is waiting in the becomeLeader() function
    leaderCondition.notify_one();
}

// Stops all threads and cleans up the thread pool
void ThreadPool::exit()
{
    // This function is responsible for shutting down the thread pool

    {
        // Acquire a lock on the queue mutex to safely modify the 'stop' flag
        std::unique_lock<std::mutex> lock(queueMutex);
        // Set the 'stop' flag to true, signaling all threads to exit
        stop = true;
    } // The lock is released here

    // Notify all threads waiting on the condition variable
    // This wakes up any threads that might be waiting for new tasks
    condition.notify_all();

    // Notify all threads waiting on the leader condition variable
    // This ensures that any thread waiting to become a leader is also notified
    leaderCondition.notify_all();

    // Iterate through all worker threads
    for (std::thread &worker : workers)
    {
        // Check if the thread is joinable (i.e., it's still running)
        if (worker.joinable())
        {
            // Wait for the thread to finish its execution
            worker.join();
        }
    }
    // After this loop, all worker threads have been properly shut down
}