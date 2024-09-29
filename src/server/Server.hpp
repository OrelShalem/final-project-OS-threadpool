#pragma once
#include <netinet/in.h>
#include <vector>
#include "../utils/ThreadPool.hpp"
#include "../common/Graph.hpp"
#include "../common/MSTFactory.hpp"
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <condition_variable>
#include <atomic>
#include <set>

class Server
{
public:
    Server(int port);
    ~Server();
    void run();
    void shutdown();

    // הוסף פונקציה סטטית ציבורית לנעילת cout
    static void lockCout() { cout_mutex.lock(); }
    static void unlockCout() { cout_mutex.unlock(); }

private:
    int serverSocket;
    int port;
    struct sockaddr_in serverAddress;
    ThreadPool threadPool;
    Graph sharedGraph;
    std::mutex sharedGraphMutex;
    std::vector<Edge> lastMST;
    bool isGraphBusy;
    std::condition_variable graphCV;
    std::atomic<bool> shouldExit{false}; // הוסף דגל יציאה

    void handleClient(int clientSocket);
    void handleInitCommand(int clientSocket, std::istringstream &iss);
    void handleMSTCommand(int clientSocket);
    void handleMetricCommand(int clientSocket, std::istringstream &iss);
    void sendResponse(int clientSocket, const std::string &message);
    void sendMenu(int clientSocket);
    bool acquireGraphLock(int clientSocket);
    void releaseGraphLock();
    void handleAddVertexCommand(int clientSocket, std::istringstream &iss);
    void handleAddEdgeCommand(int clientSocket, std::istringstream &iss);
    void handleRemoveEdgeCommand(int clientSocket, std::istringstream &iss);
    void handleRemoveVertexCommand(int clientSocket, std::istringstream &iss);

    static std::mutex cout_mutex;
};
