// This file contains the implementation of the Server class, which manages a multi-threaded server for handling graph operations.

// Include necessary headers
#include "Server.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sstream>
#include "../utils/ThreadPool.hpp"
#include "../common/Graph.hpp"
#include "../common/MSTFactory.hpp"
#include <algorithm>
#include "../common/MSTMetrics.hpp"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <sys/select.h>

// Mutex for thread-safe console output
std::mutex Server::cout_mutex;

// Constructor: Initialize the server with a given port
Server::Server(int port) : port(port), threadPool(4), isGraphBusy(false), sharedGraph(), serverSocket(-1)
{
    // Create a socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) < 0)
    {
        perror("Listen failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Print a message indicating the server is listening
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Server listening on port " << port << std::endl;
    }
}

// Destructor: Close the server socket
Server::~Server()
{
    close(serverSocket);
}

// Main server loop
void Server::run()
{
    while (!shouldExit.load(std::memory_order_acquire))
    {
        fd_set readfds;
        FD_ZERO(&readfds);

        {
            std::lock_guard<std::mutex> lock(sharedGraphMutex);
            if (serverSocket == -1)
            {
                break;
            }
            FD_SET(serverSocket, &readfds);
        }

        // Set up a timeout for select
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int maxfd;
        {
            std::lock_guard<std::mutex> lock(sharedGraphMutex);
            maxfd = serverSocket;
        }

        // Wait for activity on the socket
        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            if (errno == EINTR)
                continue;
            perror("select error");
            break;
        }

        if (activity == 0)
            continue;

        {
            std::lock_guard<std::mutex> lock(sharedGraphMutex);
            if (FD_ISSET(serverSocket, &readfds))
            {
                // Accept a new client connection
                int clientSocket = accept(serverSocket, nullptr, nullptr);
                if (clientSocket < 0)
                {
                    if (errno == EINTR || errno == EWOULDBLOCK)
                        continue;
                    if (errno == EBADF && shouldExit.load(std::memory_order_acquire))
                        break; // Server is closing, this is expected
                    perror("Accept failed");
                    break;
                }
                // Handle the client in a separate thread
                threadPool.enqueue([this, clientSocket]()
                                   { handleClient(clientSocket); });
            }
        }
    }

    // Close the server socket when exiting
    {
        std::lock_guard<std::mutex> lock(sharedGraphMutex);
        if (serverSocket != -1)
        {
            close(serverSocket);
            serverSocket = -1;
        }
    }
}

// Shutdown the server
void Server::shutdown()
{
    shouldExit.store(true, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Server is shutting down..." << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(sharedGraphMutex);
        if (serverSocket != -1)
        {
            close(serverSocket);
            serverSocket = -1;
        }
    }

    threadPool.exit();
}

// Handle individual client connections
void Server::handleClient(int clientSocket)
{
    sendMenu(clientSocket);

    while (!shouldExit.load(std::memory_order_acquire))
    {
        // Buffer to store incoming client requests
        char buffer[1024] = {0};

        // Read client request
        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0)
        {
            // Client disconnected or read error occurred
            break;
        }

        // Process the received request
        std::string request(buffer);
        // Remove newline and carriage return characters
        request.erase(std::remove(request.begin(), request.end(), '\n'), request.end());
        request.erase(std::remove(request.begin(), request.end(), '\r'), request.end());

        // Debug output
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Debug: Received request: '" << request << "'" << std::endl;
        }

        // Ignore empty requests
        if (request.empty())
        {
            continue;
        }

        // Extract the command from the request
        std::istringstream iss(request);
        std::string command;
        iss >> command;

        // Debug output for parsed command
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Debug: Parsed command: '" << command << "'" << std::endl;
        }

        // Handle different commands
        if (command == "init")
        {
            handleInitCommand(clientSocket, iss);
        }
        else if (command == "add_vtx")
        {
            handleAddVertexCommand(clientSocket, iss);
        }
        else if (command == "add_edge")
        {
            handleAddEdgeCommand(clientSocket, iss);
        }
        else if (command == "remove_edge")
        {
            handleRemoveEdgeCommand(clientSocket, iss);
        }
        else if (command == "remove_vtx")
        {
            handleRemoveVertexCommand(clientSocket, iss);
        }
        else if (command == "mst")
        {
            handleMSTCommand(clientSocket);
        }
        else if (command == "metric")
        {
            handleMetricCommand(clientSocket, iss);
        }
        else if (command == "menu")
        {
            sendMenu(clientSocket);
        }
        else if (command == "exit")
        {
            sendResponse(clientSocket, "Closing connection. Goodbye!\n");
            break;
        }
        else if (command == "change_weight")
        {
            handleChangeWeightCommand(clientSocket, iss);
        }
        else if (command == "print_graph")
        {
            handlePrintGraphCommand(clientSocket);
        }
        else
        {
            // Handle unknown commands
            sendResponse(clientSocket, "Unknown command: " + command + "\n");
            sendMenu(clientSocket);
        }
    }

    close(clientSocket);
}

void Server::handleInitCommand(int clientSocket, std::istringstream &iss)
{
    // Attempt to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    // Parse the number of vertices from the input stream
    int numVertices;
    if (!(iss >> numVertices))
    {
        sendResponse(clientSocket, "Error: Invalid number of vertices.\n");
        releaseGraphLock();
        return;
    }

    // Initialize the shared graph with the specified number of vertices
    sharedGraph = Graph(numVertices);
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Debug: Initialized shared graph with " << numVertices << " vertices." << std::endl;
    }

    // Special case: if there's only one vertex, no edges are needed
    if (numVertices == 1)
    {
        sendResponse(clientSocket, "No need to add edges.\n");
        releaseGraphLock();
        return;
    }

    sendResponse(clientSocket, "Shared graph initialized with " + std::to_string(numVertices) + " vertices. You can now add edges.\n");

    // Wait for the "edges" command from the client
    char buffer[1024] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0)
    {
        releaseGraphLock();
        return;
    }

    // Parse the "edges" command
    std::string edgeCommand(buffer);
    std::istringstream edgeIss(edgeCommand);
    std::string edgesKeyword;
    int numEdges;
    edgeIss >> edgesKeyword >> numEdges;

    // Validate the "edges" command format
    if (edgesKeyword != "edges" || edgeIss.fail())
    {
        sendResponse(clientSocket, "Invalid edge command format. Expected: edges <num_edges>\n");
        releaseGraphLock();
        return;
    }

    sendResponse(clientSocket, "Ready to receive " + std::to_string(numEdges) + " edges\n");

    // Process each edge
    for (int i = 0; i < numEdges; ++i)
    {
        memset(buffer, 0, sizeof(buffer));
        bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0)
        {
            break;
        }
        std::string edgeLine(buffer);
        std::istringstream lineStream(edgeLine);
        int source, destination, weight;
        if (lineStream >> source >> destination >> weight)
        {
            // Validate vertex numbers and add the edge to the graph
            if (source >= 0 && source < numVertices && destination >= 0 && destination < numVertices)
            {
                sharedGraph.addEdge(source, destination, weight);
                sendResponse(clientSocket, "Edge added: " + std::to_string(source) + " - " + std::to_string(destination) + " : " + std::to_string(weight) + "\n");
            }
            else
            {
                sendResponse(clientSocket, "Invalid vertex numbers. Skipping.\n");
            }
        }
        else
        {
            sendResponse(clientSocket, "Invalid edge format. Skipping.\n");
        }
    }

    // Finalize graph construction and send the current graph representation to the client
    sendResponse(clientSocket, "Graph construction complete.\n");
    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Current graph:\n" + graphRepresentation);

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to add a new vertex to the graph
void Server::handleAddVertexCommand(int clientSocket, std::istringstream &iss)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    // Add a new vertex to the graph and get its index
    int newVertexIndex = sharedGraph.getVertices();
    sharedGraph.addVertex();

    // Inform the client about the new vertex
    sendResponse(clientSocket, "New vertex added with index " + std::to_string(newVertexIndex) + "\n");
    sendResponse(clientSocket, "Enter the index of an existing vertex to connect to (0 to " + std::to_string(newVertexIndex - 1) + "): ");

    // Read the client's input for the existing vertex to connect to
    char buffer[1024] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0)
    {
        releaseGraphLock();
        return;
    }

    // Process the input
    std::string input(buffer);
    input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());

    // Convert the input to an integer
    int existingVertex;
    try
    {
        existingVertex = std::stoi(input);
    }
    catch (const std::invalid_argument &e)
    {
        sendResponse(clientSocket, "Invalid input. No edge added.\n");
        releaseGraphLock();
        return;
    }

    // If the existing vertex is valid, prompt for the edge weight
    if (existingVertex >= 0 && existingVertex < newVertexIndex)
    {
        sendResponse(clientSocket, "Enter the weight for the edge: ");
        memset(buffer, 0, sizeof(buffer));
        bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0)
        {
            releaseGraphLock();
            return;
        }

        // Process the weight input
        input = std::string(buffer);
        input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());

        // Convert the weight to an integer
        int weight;
        try
        {
            weight = std::stoi(input);
        }
        catch (const std::invalid_argument &e)
        {
            sendResponse(clientSocket, "Invalid weight. No edge added.\n");
            releaseGraphLock();
            return;
        }

        // Add the edge to the graph
        sharedGraph.addEdge(newVertexIndex, existingVertex, weight);
        sendResponse(clientSocket, "Edge added: " + std::to_string(newVertexIndex) + " - " + std::to_string(existingVertex) + " : " + std::to_string(weight) + "\n");
    }
    else
    {
        sendResponse(clientSocket, "Invalid vertex index. No edge added.\n");
    }

    // Send the updated graph representation to the client
    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to add a new edge to the graph
void Server::handleAddEdgeCommand(int clientSocket, std::istringstream &iss)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }
    // Prompt the client for edge details
    sendResponse(clientSocket, "Enter the source, destination, and weight of the edge:\n");
    int source, destination, weight;
    try
    {
        // Read the edge details from the input stream
        iss >> source >> destination >> weight;
    }
    catch (const std::invalid_argument &e)
    {
        sendResponse(clientSocket, "Invalid input. Please enter integers for source, destination, and weight.\n");
        releaseGraphLock();
        return;
    }

    // Validate the vertex indices
    if (source < 0 || source >= sharedGraph.getVertices() ||
        destination < 0 || destination >= sharedGraph.getVertices())
    {
        sendResponse(clientSocket, "Error: Invalid vertex indices.\n");
        releaseGraphLock();
        return;
    }

    // Add the edge to the graph
    sharedGraph.addEdge(source, destination, weight);
    sendResponse(clientSocket, "Edge added: " + std::to_string(source) + " - " +
                                   std::to_string(destination) + " : " + std::to_string(weight) + "\n");

    // Send the updated graph representation to the client
    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to remove an edge from the graph
void Server::handleRemoveEdgeCommand(int clientSocket, std::istringstream &iss)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    // Prompt the client for edge details
    sendResponse(clientSocket, "Enter the source and destination of the edge to remove:\n");
    int source, destination;
    iss >> source >> destination;

    // Validate the vertex indices
    if (source < 0 || source >= sharedGraph.getVertices() ||
        destination < 0 || destination >= sharedGraph.getVertices())
    {
        sendResponse(clientSocket, "Error: Invalid vertex indices.\n");
        releaseGraphLock();
        return;
    }

    // Try to remove the edge
    if (sharedGraph.removeEdge(source, destination))
    {
        sendResponse(clientSocket, "Edge removed: " + std::to_string(source) + " - " + std::to_string(destination) + "\n");
    }
    else
    {
        sendResponse(clientSocket, "Error: Edge does not exist.\n");
    }

    // Send the updated graph representation to the client
    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to remove a vertex from the graph
void Server::handleRemoveVertexCommand(int clientSocket, std::istringstream &iss)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }
    // Prompt the client for the vertex to remove
    sendResponse(clientSocket, "Enter the index of the vertex to remove:\n");

    int vertexToRemove;
    if (!(iss >> vertexToRemove))
    {
        sendResponse(clientSocket, "Error: Invalid vertex index.\n");
        releaseGraphLock();
        return;
    }

    // Validate the vertex index
    if (vertexToRemove < 0 || vertexToRemove >= sharedGraph.getVertices())
    {
        sendResponse(clientSocket, "Error: Vertex index out of range.\n");
        releaseGraphLock();
        return;
    }

    // Remove the vertex and its adjacent edges
    sharedGraph.removeVertex(vertexToRemove);
    sendResponse(clientSocket, "Vertex " + std::to_string(vertexToRemove) + " and all its adjacent edges have been removed.\n");

    // Send the updated graph representation to the client
    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to calculate the Minimum Spanning Tree (MST)
void Server::handleMSTCommand(int clientSocket)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    // Check if the graph is empty or has no edges
    if (sharedGraph.getVertices() == 0 || sharedGraph.getEdges() == 0)
    {
        sendResponse(clientSocket, "Error: Shared graph is empty or has no edges.\n");
        releaseGraphLock();
        return;
    }

    // Prompt the client to choose between Prim's and Kruskal's algorithm
    sendResponse(clientSocket, "Do you want to use Prim's or Kruskal's algorithm? (prim/kruskal)\n");

    // Read the client's choice
    char algorithmBuffer[1024] = {0};
    ssize_t algorithmBytesRead = read(clientSocket, algorithmBuffer, sizeof(algorithmBuffer) - 1);
    if (algorithmBytesRead <= 0)
    {
        releaseGraphLock();
        return;
    }
    std::string mstType(algorithmBuffer);
    mstType.erase(std::remove(mstType.begin(), mstType.end(), '\n'), mstType.end());

    // Validate the algorithm choice
    if (mstType != "prim" && mstType != "kruskal")
    {
        sendResponse(clientSocket, "Invalid algorithm type. Please use 'prim' or 'kruskal'.\n");
        releaseGraphLock();
        return;
    }

    std::vector<Edge> mst;
    try
    {
        // Create the appropriate MST algorithm and find the MST
        auto mstAlgorithm = MSTFactory::createMST(mstType);
        mst = mstAlgorithm->findMST(sharedGraph);
        lastMST = mst;
    }
    catch (const std::exception &e)
    {
        sendResponse(clientSocket, std::string("Error calculating MST: ") + e.what() + "\n");
        releaseGraphLock();
        return;
    }

    // Prepare the response string
    std::ostringstream oss;
    oss << mstType << "\n";
    oss << "MST created using " << (mstType == "prim" ? "Prim's" : "Kruskal's") << " algorithm.\n";

    // Create a map to store the tree structure
    std::unordered_map<int, std::vector<std::pair<int, int>>> tree;
    for (const auto &edge : mst)
    {
        tree[edge.source].push_back({edge.destination, edge.weight});
        tree[edge.destination].push_back({edge.source, edge.weight});
    }

    // Helper function to print the tree recursively
    std::function<void(int, int, std::string, std::unordered_set<int> &)> printTree =
        [&](int node, int parent, std::string prefix, std::unordered_set<int> &visited)
    {
        if (visited.find(node) != visited.end())
            return;
        visited.insert(node);

        bool isLast = (node == mst.back().destination);
        oss << prefix << (isLast ? "└" : "├") << "─ Node " << node;

        if (parent != -1)
        {
            auto it = std::find_if(tree[node].begin(), tree[node].end(),
                                   [parent](const auto &p)
                                   { return p.first == parent; });
            if (it != tree[node].end())
            {
                oss << " [weight: " << it->second << "]";
            }
        }
        oss << "\n";

        prefix += isLast ? "   " : "│  ";

        for (const auto &child : tree[node])
        {
            if (child.first != parent)
            {
                printTree(child.first, node, prefix, visited);
            }
        }
    };

    // Print the tree starting from the first node in the MST
    std::unordered_set<int> visited;
    printTree(mst[0].source, -1, "", visited);

    // Send the MST representation to the client
    sendResponse(clientSocket, oss.str());

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to calculate various metrics of the MST
void Server::handleMetricCommand(int clientSocket, std::istringstream &iss)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    // Check if the graph is initialized
    if (sharedGraph.getVertices() == 0)
    {
        sendResponse(clientSocket, "Error: Shared graph not initialized.\n");
        releaseGraphLock();
        return;
    }

    // Check if an MST has been calculated
    if (lastMST.empty())
    {
        sendResponse(clientSocket, "Error: No MST calculated yet. Please use 'mst' command first.\n");
        releaseGraphLock();
        return;
    }

    // Read the metric type from the input stream
    std::string metricType;
    iss >> metricType;

    // Calculate the requested metric
    std::string response;
    if (metricType == "total_weight")
    {
        response = "Total weight of MST: " + std::to_string(MSTMetrics::getTotalWeight(lastMST)) + "\n";
    }
    else if (metricType == "longest_path")
    {
        response = "Longest path in MST: " + std::to_string(MSTMetrics::getLongestDistance(sharedGraph, lastMST)) + "\n";
    }
    else if (metricType == "average_path")
    {
        response = "Average path length in MST: " + std::to_string(MSTMetrics::getAverageDistance(sharedGraph, lastMST)) + "\n";
    }
    else if (metricType == "shortest_path")
    {
        response = "Shortest path in MST: " + std::to_string(MSTMetrics::getShortestDistance(lastMST)) + "\n";
    }
    else
    {
        response = "Unknown metric type: " + metricType + "\n";
    }

    // Send the metric result to the client
    sendResponse(clientSocket, response);

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to change the weight of an edge
void Server::handleChangeWeightCommand(int clientSocket, std::istringstream &iss)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    // Read the edge details and new weight from the input stream
    int source, destination, newWeight;
    if (!(iss >> source >> destination >> newWeight))
    {
        sendResponse(clientSocket, "Error: Invalid input for change_weight command.\n");
        releaseGraphLock();
        return;
    }

    // Try to change the weight of the edge
    if (sharedGraph.changeWeight(source, destination, newWeight))
    {
        sendResponse(clientSocket, "Edge weight changed: " + std::to_string(source) + " - " +
                                       std::to_string(destination) + " : " + std::to_string(newWeight) + "\n");
    }
    else
    {
        sendResponse(clientSocket, "Error: Edge not found or invalid vertices.\n");
    }

    // Release the graph lock
    releaseGraphLock();
}

// This function handles the command to print the current state of the graph
void Server::handlePrintGraphCommand(int clientSocket)
{
    // Try to acquire the graph lock, return if unsuccessful
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    // Get the string representation of the graph
    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Current graph:\n" + graphRepresentation);

    // Release the graph lock
    releaseGraphLock();
}

// This function sends a response to the client
void Server::sendResponse(int clientSocket, const std::string &message)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    write(clientSocket, message.c_str(), message.size());
    std::cout << "Sent to client: " << message; // Log the message if desired
}

void Server::sendMenu(int clientSocket)
{
    std::ostringstream menuStream;
    menuStream << "\n=== MST Server Menu ===\n"
               << "Available commands:\n"
               << "1. init <num_vertices> - Initialize a new graph with the specified number of vertices\n"
               << "2. add_vtx - Add a new vertex to the graph\n"
               << "3. mst - Calculate the Minimum Spanning Tree (you'll be prompted to choose Prim's or Kruskal's algorithm)\n"
               << "3. metric <metric_type> - Calculate a metric for the last computed MST\n"
               << "   Available metrics:\n"
               << "   - total_weight: Total weight of the MST\n"
               << "   - longest_path: Longest path in the MST\n"
               << "   - average_path: Average path length in the MST\n"
               << "   - shortest_path: Shortest path between any two vertices in the MST\n"
               << "4. menu - Display this menu again\n"
               << "5. exit - Close the connection\n"
               << "\nEnter your command: ";

    sendResponse(clientSocket, menuStream.str());
}

// This function attempts to acquire a lock on the shared graph
bool Server::acquireGraphLock(int clientSocket)
{
    // Create a unique lock on the shared graph mutex
    std::unique_lock<std::mutex> lock(sharedGraphMutex);

    // Check if the graph is already in use
    if (isGraphBusy)
    {
        // If busy, send an error message to the client
        sendResponse(clientSocket, "Error: Graph is currently in use by another client. Please try again later.\n");
        return false; // Return false to indicate lock acquisition failure
    }

    // If not busy, set the graph as busy
    isGraphBusy = true;
    return true; // Return true to indicate successful lock acquisition
}

// This function releases the lock on the shared graph
void Server::releaseGraphLock()
{
    // Create a lock guard for the shared graph mutex
    std::lock_guard<std::mutex> lock(sharedGraphMutex);

    // Set the graph as not busy
    isGraphBusy = false;

    // Notify one waiting thread that the graph is now available
    graphCV.notify_one();
}