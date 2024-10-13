#ifndef SERVER_HPP
#define SERVER_HPP

#include "../utils/threadpool.hpp"

class ServerClient : public Client
{
public:
    ServerClient(int socket, std::shared_ptr<Graph> graph, ThreadPool &pool);
    void handle() override;

private:
    std::mutex clientMutex;
    ThreadPool &threadPool;
    void sendMenu();
    std::string receiveChoice();
    void sendResponse(const std::string &response);
    void handleChoice(const std::string &choice);
    void buildGraphFromClientInput();
    void handleAddVertex();
    void handleAddEdge();
    void handleRemoveVertex();
    void handleRemoveEdge();
    void computeMST();
    void handleMSTQueries();
    void printGraph();
};

#endif // SERVER_HPP
