#include "server/Server.hpp"
#include <csignal>
#include <thread>
#include <iostream>
#include <string>

// Global pointer to the Server instance for signal handling
Server *globalServerPtr = nullptr;

// Signal handler function to gracefully shut down the server
void signalHandler(int signum)
{
    if (globalServerPtr)
    {
        globalServerPtr->shutdown();
    }
    std::exit(signum);
}

// Function to handle console input in a separate thread
void consoleInputThread(Server &server)
{
    std::string input;
    while (true)
    {
        std::getline(std::cin, input);
        if (input == "exit")
        {
            Server::lockCout();
            std::cout << "Shutting down server..." << std::endl;
            Server::unlockCout();
            server.shutdown();
            break;
        }
    }
}

int main()
{
    {
        // Create a Server instance listening on port 9036
        Server server(9036);
        globalServerPtr = &server;

        // Register signal handlers for SIGINT and SIGTERM
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // Start a separate thread to handle console input
        std::thread inputThread(consoleInputThread, std::ref(server));

        // Run the server
        server.run();

        // Wait for the input thread to finish
        if (inputThread.joinable())
        {
            inputThread.join();
        }
    }

    // Reset the global server pointer
    globalServerPtr = nullptr;
    return 0;
}