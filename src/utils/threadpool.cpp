// This file implements the ThreadPool class and related components for managing concurrent client connections in a server application.

// Include necessary headers
#include "threadpool.hpp"
#include "../server/server.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <string>
#include <sstream>

// Global mutex for thread-safe console output
std::mutex coutMutex;

// Client class implementation
Client::Client(int socket, std::shared_ptr<Graph> graph) : socket_(socket), connected_(true), graph_(graph) {}

bool Client::isConnected() { return connected_; }

Client::~Client()
{
    if (socket_ != -1)
    {
        close(socket_);
        socket_ = -1;
    }
}

// LeaderTask class implementation
LeaderTask::LeaderTask(int serverSocket) : Client(-1, nullptr), serverSocket(serverSocket) {}
void LeaderTask::handle() {}
bool LeaderTask::isConnected() { return false; }

// ThreadPool class implementation
ThreadPool::ThreadPool(size_t numThreads) : stop_flag(false), sharedGraph(std::make_shared<Graph>())
{
    // Create worker threads
    for (size_t i = 0; i < numThreads; ++i)
    {
        workers.emplace_back(&ThreadPool::workerThread, this);
    }
}

ThreadPool::~ThreadPool()
{
    stop();
}

// Start the server and initialize the leader thread
void ThreadPool::start()
{
    // Create and configure server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Set socket options
    int reuse = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
        close(serverSocket);
        return;
    }

    // Bind socket to address and port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(9039);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Failed to bind to port 9039" << std::endl;
        close(serverSocket);
        return;
    }

    // Start listening for connections
    if (listen(serverSocket, 3) < 0)
    {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Server is listening on port 9039" << std::endl;

    // Start the leader thread
    leader = std::thread(&ThreadPool::leaderThread, this, serverSocket);
}

// Stop the thread pool and clean up resources
void ThreadPool::stop()
{
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop_flag = true;
        condition.notify_all();
    }

    // Close the server socket
    if (serverSocket != -1)
    {
        close(serverSocket);
        serverSocket = -1;
    }

    // Join all worker threads
    for (std::thread &worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    // Join the leader thread
    if (leader.joinable())
    {
        leader.join();
    }
}

// Add a new client to the queue for handling
void ThreadPool::handleNewClient(std::shared_ptr<Client> client)
{
    {
        // Acquire a lock on the queue mutex to ensure thread-safe access
        std::unique_lock<std::mutex> lock(queueMutex);

        // Add the new client to the queue of clients waiting to be handled
        clients.push(client);

        // Notify one waiting worker thread that a new client is available
        condition.notify_one();
    }
}

// Worker thread function
void ThreadPool::workerThread()
{
    // Main loop for the worker thread
    while (!stop_flag)
    {
        std::shared_ptr<Client> client;
        {
            // Acquire lock on the queue mutex
            std::unique_lock<std::mutex> lock(queueMutex);

            // Wait for a client or stop signal
            condition.wait(lock, [this]
                           { return stop_flag || !clients.empty(); });

            // Exit if stopping and no clients left
            if (stop_flag && clients.empty())
            {
                return;
            }

            // Get the next client from the queue
            client = std::move(clients.front());
            clients.pop();
        }

        // Check if this is a special LeaderTask
        if (auto leaderTask = std::dynamic_pointer_cast<LeaderTask>(client))
        {
            // If so, become the leader thread
            leaderThread(leaderTask->serverSocket);
        }
        else
        {
            // Regular client handling
            std::cout << "Worker thread " << std::this_thread::get_id() << " is handling a client" << std::endl;

            // Handle client requests while connected
            while (client->isConnected())
            {
                client->handle();
            }

            std::cout << "Client disconnected, thread " << std::this_thread::get_id() << " is free again." << std::endl;
        }
    }
}

// Leader thread function
// Leader thread function
void ThreadPool::leaderThread(int serverSocket)
{
    // Main loop for the leader thread
    while (!stop_flag)
    {
        // Set up file descriptor set for select()
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);

        // Set up timeout for select()
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // Wait for activity on the server socket with a timeout
        int activity = select(serverSocket + 1, &readfds, NULL, NULL, &timeout);

        // Check if the thread should stop
        if (stop_flag)
            break;

        // Handle select() errors
        if (activity < 0)
        {
            if (errno != EINTR)
            {
                std::cerr << "Select error" << std::endl;
            }
            continue;
        }

        // If select() timed out, continue to the next iteration
        if (activity == 0)
            continue;

        // Accept new client connection
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);

        // Handle accept() errors
        if (clientSocket < 0)
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                safePrint("Failed to accept client connection");
            }
            continue;
        }

        // Log the new client connection
        std::ostringstream oss;
        oss << "Leader thread " << std::this_thread::get_id() << " accepted a new client connection";
        safePrint(oss.str());

        // Create a new ServerClient object for the accepted connection
        auto newClient = std::make_shared<ServerClient>(clientSocket, sharedGraph, *this);

        // Make one of the worker threads the new leader
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            clients.push(std::make_shared<LeaderTask>(serverSocket));
            condition.notify_one();
        }

        // Log that this thread is now handling the client
        oss.str("");
        oss << "Thread " << std::this_thread::get_id() << " is handling the client";
        safePrint(oss.str());

        // Handle client requests while connected and not stopping
        while (newClient->isConnected() && !stop_flag)
        {
            newClient->handle();
        }

        // Log that the client has disconnected
        oss.str("");
        oss << "Client disconnected, thread " << std::this_thread::get_id() << " is free again.";
        safePrint(oss.str());

        // Return to being a worker thread
        return;
    }
}

// Lock the shared graph
void ThreadPool::lockGraph()
{
    graphMutex.lock();
}

// Unlock the shared graph
void ThreadPool::unlockGraph()
{
    graphMutex.unlock();
}

// Get the graph mutex
std::mutex &ThreadPool::getGraphMutex()
{
    return graphMutex;
}
