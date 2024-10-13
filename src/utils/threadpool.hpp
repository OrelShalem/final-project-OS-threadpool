#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include "../common/Graph.hpp"

class MSTFactory;
class MSTMetrics;

class Client
{
public:
    Client(int socket, std::shared_ptr<Graph> graph);
    virtual void handle() = 0;
    virtual bool isConnected();
    virtual ~Client();

protected:
    int socket_;
    std::atomic<bool> connected_;
    std::shared_ptr<Graph> graph_;
};

class LeaderTask : public Client
{
public:
    LeaderTask(int serverSocket);
    void handle() override;
    bool isConnected() override;
    int serverSocket;
};

class ServerClient;

class ThreadPool
{
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void start();
    void stop();
    void handleNewClient(std::shared_ptr<Client> client);

    void lockGraph();
    void unlockGraph();
    std::mutex &getGraphMutex();

private:
    void workerThread();
    void leaderThread(int serverSocket);

    std::vector<std::thread> workers;
    std::queue<std::shared_ptr<Client>> clients;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop_flag;
    std::thread leader;
    std::shared_ptr<Graph> sharedGraph;
    int serverSocket = -1;
    std::mutex graphMutex;
};

extern std::mutex coutMutex;

template <typename T>
void safePrint(const T &message)
{
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << message << std::endl;
}

#endif // THREADPOOL_HPP
