#include "server/Server.hpp"
#include <csignal>
#include <thread>
#include <iostream>
#include <string>

// הסר את ההגדרה של cout_mutex מכאן

Server *globalServerPtr = nullptr;

void signalHandler(int signum)
{
    if (globalServerPtr)
    {
        globalServerPtr->shutdown();
    }
    std::exit(signum);
}

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
    Server server(9036);
    globalServerPtr = &server;

    // רשום את מטפל הסיגנל
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // הפעל thread נפרד לקליטת קלט מהקונסול
    std::thread inputThread(consoleInputThread, std::ref(server));

    server.run();

    // חכה ל-thread של הקלט לסיים
    if (inputThread.joinable())
    {
        inputThread.join();
    }

    return 0;
}