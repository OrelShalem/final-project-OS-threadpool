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

std::mutex Server::cout_mutex;

Server::Server(int port) : port(port), threadPool(4), isGraphBusy(false), sharedGraph(), serverSocket(-1)
{
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) < 0)
    {
        perror("Listen failed");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Server listening on port " << port << std::endl;
    }
}

Server::~Server()
{
    close(serverSocket);
}

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

        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int maxfd;
        {
            std::lock_guard<std::mutex> lock(sharedGraphMutex);
            maxfd = serverSocket;
        }

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
                int clientSocket = accept(serverSocket, nullptr, nullptr);
                if (clientSocket < 0)
                {
                    if (errno == EINTR || errno == EWOULDBLOCK)
                        continue;
                    if (errno == EBADF && shouldExit.load(std::memory_order_acquire))
                        break; // השרת נסגר, זה צפוי
                    perror("Accept failed");
                    break;
                }
                threadPool.enqueue([this, clientSocket]()
                                   { handleClient(clientSocket); });
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(sharedGraphMutex);
        if (serverSocket != -1)
        {
            close(serverSocket);
            serverSocket = -1;
        }
    }
}

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

void Server::handleClient(int clientSocket)
{
    sendMenu(clientSocket);

    while (!shouldExit.load(std::memory_order_acquire))
    {
        char buffer[1024] = {0};
        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0)
        {
            // הלקוח התנתק או שיש שגיאת קריאה
            break;
        }

        std::string request(buffer);
        request.erase(std::remove(request.begin(), request.end(), '\n'), request.end());
        request.erase(std::remove(request.begin(), request.end(), '\r'), request.end());

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Debug: Received request: '" << request << "'" << std::endl;
        }

        if (request.empty())
        {
            continue; // התעלם מקלט ריק
        }

        std::istringstream iss(request);
        std::string command;
        iss >> command;

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "Debug: Parsed command: '" << command << "'" << std::endl;
        }

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
            sendResponse(clientSocket, "Unknown command: " + command + "\n");
            sendMenu(clientSocket);
        }
    }

    close(clientSocket);
}

void Server::handleInitCommand(int clientSocket, std::istringstream &iss)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    int numVertices;
    if (!(iss >> numVertices))
    {
        sendResponse(clientSocket, "Error: Invalid number of vertices.\n");
        releaseGraphLock();
        return;
    }

    sharedGraph = Graph(numVertices);
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "Debug: Initialized shared graph with " << numVertices << " vertices." << std::endl;
    }
    if (numVertices == 1)
    {
        sendResponse(clientSocket, "No need to add edges.\n");
        releaseGraphLock();
        return;
    }

    sendResponse(clientSocket, "Shared graph initialized with " + std::to_string(numVertices) + " vertices. You can now add edges.\n");

    // Wait for the "edges" command
    char buffer[1024] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0)
    {
        releaseGraphLock();
        return;
    }

    std::string edgeCommand(buffer);
    std::istringstream edgeIss(edgeCommand);
    std::string edgesKeyword;
    int numEdges;
    edgeIss >> edgesKeyword >> numEdges;

    if (edgesKeyword != "edges" || edgeIss.fail())
    {
        sendResponse(clientSocket, "Invalid edge command format. Expected: edges <num_edges>\n");
        releaseGraphLock();
        return;
    }

    sendResponse(clientSocket, "Ready to receive " + std::to_string(numEdges) + " edges\n");

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

    sendResponse(clientSocket, "Graph construction complete.\n");
    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Current graph:\n" + graphRepresentation);

    releaseGraphLock();
}

void Server::handleAddVertexCommand(int clientSocket, std::istringstream &iss)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    int newVertexIndex = sharedGraph.getVertices();
    sharedGraph.addVertex();

    sendResponse(clientSocket, "New vertex added with index " + std::to_string(newVertexIndex) + "\n");
    sendResponse(clientSocket, "Enter the index of an existing vertex to connect to (0 to " + std::to_string(newVertexIndex - 1) + "): ");

    char buffer[1024] = {0};
    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0)
    {
        releaseGraphLock();
        return;
    }

    std::string input(buffer);
    input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());

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

        input = std::string(buffer);
        input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());

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

        sharedGraph.addEdge(newVertexIndex, existingVertex, weight);
        sendResponse(clientSocket, "Edge added: " + std::to_string(newVertexIndex) + " - " + std::to_string(existingVertex) + " : " + std::to_string(weight) + "\n");
    }
    else
    {
        sendResponse(clientSocket, "Invalid vertex index. No edge added.\n");
    }

    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    releaseGraphLock();
}

void Server::handleAddEdgeCommand(int clientSocket, std::istringstream &iss)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }
    // print format: add_edge <source> <destination> <weight>
    sendResponse(clientSocket, "Enter the source, destination, and weight of the edge:\n");
    int source, destination, weight;
    try
    {
        iss >> source >> destination >> weight;
    }
    catch (const std::invalid_argument &e)
    {
        sendResponse(clientSocket, "Invalid input. Please enter integers for source, destination, and weight.\n");
        releaseGraphLock();
        return;
    }

    if (source < 0 || source >= sharedGraph.getVertices() ||
        destination < 0 || destination >= sharedGraph.getVertices())
    {
        sendResponse(clientSocket, "Error: Invalid vertex indices.\n");
        releaseGraphLock();
        return;
    }

    sharedGraph.addEdge(source, destination, weight);
    sendResponse(clientSocket, "Edge added: " + std::to_string(source) + " - " +
                                   std::to_string(destination) + " : " + std::to_string(weight) + "\n");

    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    releaseGraphLock();
}

void Server::handleRemoveEdgeCommand(int clientSocket, std::istringstream &iss)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    sendResponse(clientSocket, "Enter the source and destination of the edge to remove:\n");
    int source, destination;
    iss >> source >> destination;

    if (source < 0 || source >= sharedGraph.getVertices() ||
        destination < 0 || destination >= sharedGraph.getVertices())
    {
        sendResponse(clientSocket, "Error: Invalid vertex indices.\n");
        releaseGraphLock();
        return;
    }

    if (sharedGraph.removeEdge(source, destination))
    {
        sendResponse(clientSocket, "Edge removed: " + std::to_string(source) + " - " + std::to_string(destination) + "\n");
    }
    else
    {
        sendResponse(clientSocket, "Error: Edge does not exist.\n");
    }

    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    releaseGraphLock();
}

void Server::handleRemoveVertexCommand(int clientSocket, std::istringstream &iss)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }
    sendResponse(clientSocket, "Enter the index of the vertex to remove:\n");

    int vertexToRemove;
    if (!(iss >> vertexToRemove))
    {
        sendResponse(clientSocket, "Error: Invalid vertex index.\n");
        releaseGraphLock();
        return;
    }

    if (vertexToRemove < 0 || vertexToRemove >= sharedGraph.getVertices())
    {
        sendResponse(clientSocket, "Error: Vertex index out of range.\n");
        releaseGraphLock();
        return;
    }

    sharedGraph.removeVertex(vertexToRemove);
    sendResponse(clientSocket, "Vertex " + std::to_string(vertexToRemove) + " and all its adjacent edges have been removed.\n");

    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Updated graph:\n" + graphRepresentation);

    releaseGraphLock();
}

void Server::handleMSTCommand(int clientSocket)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    if (sharedGraph.getVertices() == 0 || sharedGraph.getEdges() == 0)
    {
        sendResponse(clientSocket, "Error: Shared graph is empty or has no edges.\n");
        releaseGraphLock();
        return;
    }

    sendResponse(clientSocket, "Do you want to use Prim's or Kruskal's algorithm? (prim/kruskal)\n");

    char algorithmBuffer[1024] = {0};
    ssize_t algorithmBytesRead = read(clientSocket, algorithmBuffer, sizeof(algorithmBuffer) - 1);
    if (algorithmBytesRead <= 0)
    {
        releaseGraphLock();
        return;
    }
    std::string mstType(algorithmBuffer);
    mstType.erase(std::remove(mstType.begin(), mstType.end(), '\n'), mstType.end());

    if (mstType != "prim" && mstType != "kruskal")
    {
        sendResponse(clientSocket, "Invalid algorithm type. Please use 'prim' or 'kruskal'.\n");
        releaseGraphLock();
        return;
    }

    std::vector<Edge> mst;
    try
    {
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

    sendResponse(clientSocket, oss.str());

    releaseGraphLock();
}

void Server::handleMetricCommand(int clientSocket, std::istringstream &iss)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    if (sharedGraph.getVertices() == 0)
    {
        sendResponse(clientSocket, "Error: Shared graph not initialized.\n");
        releaseGraphLock();
        return;
    }

    if (lastMST.empty())
    {
        sendResponse(clientSocket, "Error: No MST calculated yet. Please use 'mst' command first.\n");
        releaseGraphLock();
        return;
    }

    std::string metricType;
    iss >> metricType;

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

    sendResponse(clientSocket, response);

    releaseGraphLock();
}

void Server::handleChangeWeightCommand(int clientSocket, std::istringstream &iss)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    int source, destination, newWeight;
    if (!(iss >> source >> destination >> newWeight))
    {
        sendResponse(clientSocket, "Error: Invalid input for change_weight command.\n");
        releaseGraphLock();
        return;
    }

    if (sharedGraph.changeWeight(source, destination, newWeight))
    {
        sendResponse(clientSocket, "Edge weight changed: " + std::to_string(source) + " - " +
                                       std::to_string(destination) + " : " + std::to_string(newWeight) + "\n");
    }
    else
    {
        sendResponse(clientSocket, "Error: Edge not found or invalid vertices.\n");
    }

    releaseGraphLock();
}

void Server::handlePrintGraphCommand(int clientSocket)
{
    if (!acquireGraphLock(clientSocket))
    {
        return;
    }

    std::string graphRepresentation = sharedGraph.toString();
    sendResponse(clientSocket, "Current graph:\n" + graphRepresentation);

    releaseGraphLock();
}

void Server::sendResponse(int clientSocket, const std::string &message)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    write(clientSocket, message.c_str(), message.size());
    std::cout << "Sent to client: " << message; // אם אתה רוצה לרשום את ההודעה
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

bool Server::acquireGraphLock(int clientSocket)
{
    std::unique_lock<std::mutex> lock(sharedGraphMutex);
    if (isGraphBusy)
    {
        sendResponse(clientSocket, "Error: Graph is currently in use by another client. Please try again later.\n");
        return false;
    }
    isGraphBusy = true;
    return true;
}

void Server::releaseGraphLock()
{
    std::lock_guard<std::mutex> lock(sharedGraphMutex);
    isGraphBusy = false;
    graphCV.notify_one();
}