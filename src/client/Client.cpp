// Include necessary headers
#include "Client.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <vector>

// Constructor initializes Client with server address and port
Client::Client(const std::string &address, int port) : address(address), port(port), sock(-1) {}

// Destructor closes the socket if it's open
Client::~Client()
{
    if (sock != -1)
    {
        close(sock);
    }
}

// Establishes a connection to the server
void Client::connectToServer()
{
    if (sock == -1)
    {
        // Create a new socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            std::cerr << "Socket creation error" << std::endl;
            return;
        }

        // Set up the server address structure
        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        // Convert IP address from string to binary form
        if (inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr) <= 0)
        {
            std::cerr << "Invalid address" << std::endl;
            return;
        }

        // Connect to the server
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            std::cerr << "Connection failed" << std::endl;
            return;
        }

        // Receive and display the initial menu from the server
        std::string initialMenu = receiveResponse();
        std::cout << initialMenu;
    }
}

// Sends a request to the server and handles the response
void Client::sendRequest(const std::string &request)
{
    connectToServer();

    // Debug output
    std::cout << "Debug: Sending request: '" << request << "'" << std::endl;

    // Send the request to the server
    send(sock, request.c_str(), request.size(), 0);

    // Receive and display the server's response
    std::string response = receiveResponse();
    std::cout << response;

    // Handle different types of responses

    // If the graph is in use, offer to retry
    if (response.find("Error: Graph is currently in use") != std::string::npos)
    {
        std::cout << "The shared graph is currently in use. Would you like to retry? (y/n): ";
        std::string answer;
        std::getline(std::cin, answer);
        if (answer == "y" || answer == "Y")
        {
            sendRequest(request); // Retry the request
            return;
        }
    }

    // Handle graph initialization
    if (response.find("Shared graph initialized with") != std::string::npos)
    {
        int numEdges;
        std::cout << "Enter the number of edges: ";
        std::cin >> numEdges;
        std::cin.ignore(); // Clear the newline character
        if (numEdges == 0)
        {
            std::cout << "No edges to add." << std::endl;
            return;
        }

        std::string edgeCommand = "edges " + std::to_string(numEdges) + "\n";
        send(sock, edgeCommand.c_str(), edgeCommand.length(), 0);
        std::cout << receiveResponse();

        std::cout << "Enter edges in the format: <source> <destination> <weight> (one per line):" << std::endl;
        for (int i = 0; i < numEdges; ++i)
        {
            std::string edgeLine;
            std::getline(std::cin, edgeLine);
            send(sock, (edgeLine + "\n").c_str(), edgeLine.size() + 1, 0);
            std::cout << receiveResponse();
        }

        std::cout << receiveResponse(); // Receive final graph representation
    }
    else if (response.find("Do you want to use Prim's or Kruskal's algorithm?") != std::string::npos)
    {
        std::string algorithm;
        std::getline(std::cin, algorithm);
        send(sock, (algorithm + "\n").c_str(), algorithm.size() + 1, 0);
        std::cout << receiveResponse();
    }
    // Handle MST creation and offer additional metrics
    else if (response.find("MST created using") != std::string::npos)
    {
        std::cout << response; // הדפס את כל התגובה כפי שהיא
        // After receiving MST, offer additional metrics
        std::cout << "Do you want to calculate additional metrics? (y/n): ";
        std::string answer;
        std::getline(std::cin, answer);
        if (answer == "y" || answer == "Y")
        {
            sendMetricsRequests();
        }
    }
    else if (request.find("add_vtx") != std::string::npos ||
             request.find("add_edge") != std::string::npos ||
             request.find("remove_edge") != std::string::npos ||
             request.find("remove_vtx") != std::string::npos)
    {
        // Handle add_vtx, add_edge, remove_edge, remove_vtx
        if (response.find("Enter") != std::string::npos)
        {
            std::string input;
            std::getline(std::cin, input);
            send(sock, (input + "\n").c_str(), input.size() + 1, 0);
            std::cout << receiveResponse();
        }
        // No need for additional input, just return to main menu
    }
}

// Sends requests for various metrics
void Client::sendMetricsRequests()
{
    const std::vector<std::string> metrics = {
        "total_weight",
        "longest_path",
        "average_path",
        "shortest_path"};

    for (const auto &metric : metrics)
    {
        std::string metricRequest = "metric " + metric + "\n";
        send(sock, metricRequest.c_str(), metricRequest.size(), 0);
        std::string response = receiveResponse();
        std::cout << response;
    }
}

// Receives a response from the server
std::string Client::receiveResponse()
{
    char buffer[1024] = {0};
    std::string fullResponse;
    ssize_t valread;
    fd_set readfds;
    struct timeval tv;
    int retval;

    do
    {
        // Set up the file descriptor set and timeout for select()
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout

        // Wait for the socket to be ready for reading
        retval = select(sock + 1, &readfds, NULL, NULL, &tv);
        if (retval == -1)
        {
            perror("select()");
            break;
        }
        else if (retval)
        {
            // Read data from the socket
            valread = read(sock, buffer, sizeof(buffer) - 1);
            if (valread > 0)
            {
                buffer[valread] = '\0';
                fullResponse += buffer;
            }
            else
            {
                break;
            }
        }
        else
        {
            break; // Timeout occurred
        }
    } while (true);

    return fullResponse;
}