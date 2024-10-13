// This file implements the server-side functionality for a graph manipulation and MST computation application.

// Include necessary headers
#include "server.hpp"
#include "../utils/threadpool.hpp"
#include "../common/MSTFactory.hpp"
#include "../common/MSTMetrics.hpp"
#include <iostream>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <thread>

// Global flag to control server running state
std::atomic<bool> running(true);

// ServerClient class implementation
ServerClient::ServerClient(int socket, std::shared_ptr<Graph> graph, ThreadPool &pool)
    : Client(socket, graph), threadPool(pool) {}

// Main handler for client connections
void ServerClient::handle()
{
    std::lock_guard<std::mutex> lock(clientMutex);
    sendMenu(); // Send menu immediately after connection
    while (connected_)
    {
        std::string choice = receiveChoice();
        if (choice.empty()) // If client disconnected
        {
            break;
        }
        handleChoice(choice);
        if (choice == "exit" || choice == "9")
        {
            break;
        }
        sendMenu(); // Send menu again after each choice
    }
}

// Send menu options to the client
void ServerClient::sendMenu()
{
    std::string menu = "Choose an option:\n"
                       "1. build_graph\n"
                       "2. add_vertex\n"
                       "3. add_edge\n"
                       "4. remove_vertex\n"
                       "5. remove_edge\n"
                       "6. compute_mst\n"
                       "7. query_mst\n"
                       "8. print_graph\n"
                       "9. exit\n";
    sendResponse(menu);
}

// Receive client's choice
std::string ServerClient::receiveChoice()
{
    char buffer[1024] = {0};
    int valread = read(socket_, buffer, 1024);
    if (valread <= 0)
    {
        connected_ = false;
        return "";
    }
    return std::string(buffer);
}

// Send response to the client
void ServerClient::sendResponse(const std::string &response)
{
    send(socket_, response.c_str(), response.length(), 0);
}

// Handle client's choice
void ServerClient::handleChoice(const std::string &choice)
{
    threadPool.lockGraph();
    std::lock_guard<std::mutex> lock(threadPool.getGraphMutex(), std::adopt_lock);

    // Call appropriate function based on client's choice
    if (choice == "build_graph" || choice == "1")
    {
        buildGraphFromClientInput();
    }
    else if (choice == "add_vertex" || choice == "2")
    {
        handleAddVertex();
    }
    else if (choice == "add_edge" || choice == "3")
    {
        handleAddEdge();
    }
    else if (choice == "remove_vertex" || choice == "4")
    {
        handleRemoveVertex();
    }
    else if (choice == "remove_edge" || choice == "5")
    {
        handleRemoveEdge();
    }
    else if (choice == "compute_mst" || choice == "6")
    {
        computeMST();
    }
    else if (choice == "query_mst" || choice == "7")
    {
        handleMSTQueries();
    }
    else if (choice == "print_graph" || choice == "8")
    {
        printGraph();
    }
    else if (choice == "exit" || choice == "9")
    {
        sendResponse("Goodbye!");
        connected_ = false;
    }
    else
    {
        sendResponse("Invalid choice. Please try again.");
    }
}

// Build graph from client input
void ServerClient::buildGraphFromClientInput()
{
    sendResponse("Enter the number of vertices: ");
    int numVertices = std::stoi(receiveChoice());
    graph_->clear();
    for (int i = 0; i < numVertices; ++i)
    {
        graph_->addVertex();
    }

    sendResponse("Enter the number of edges: ");
    int numEdges = std::stoi(receiveChoice());

    sendResponse("Enter " + std::to_string(numEdges) + " edges in the format 'source destination weight':");
    for (int i = 0; i < numEdges; ++i)
    {
        std::string edgeInfo = receiveChoice();
        std::istringstream iss(edgeInfo);
        int source, destination, weight;
        if (iss >> source >> destination >> weight)
        {
            graph_->addEdge(source, destination, weight);
        }
        else
        {
            sendResponse("Invalid edge format. Skipping this edge.");
        }
    }

    sendResponse("Graph built successfully.");
}

void ServerClient::handleAddVertex()
{
    int newVertex = graph_->addVertex();
    sendResponse("Vertex " + std::to_string(newVertex) + " added successfully.");
}

// Handle adding an edge
void ServerClient::handleAddEdge()
{
    sendResponse("Enter source vertex: ");
    int source = std::stoi(receiveChoice());
    sendResponse("Enter destination vertex: ");
    int destination = std::stoi(receiveChoice());
    sendResponse("Enter weight: ");
    int weight = std::stoi(receiveChoice());
    graph_->addEdge(source, destination, weight);
    sendResponse("Edge added successfully.");
}

// Handle removing a vertex
void ServerClient::handleRemoveVertex()
{
    sendResponse("Enter vertex to remove: ");
    int vertex = std::stoi(receiveChoice());
    if (graph_->removeVertex(vertex))
    {
        sendResponse("Vertex removed successfully.");
    }
    else
    {
        sendResponse("Failed to remove vertex.");
    }
}

// Handle removing an edge
void ServerClient::handleRemoveEdge()
{
    sendResponse("Enter source vertex: ");
    int source = std::stoi(receiveChoice());
    sendResponse("Enter destination vertex: ");
    int destination = std::stoi(receiveChoice());
    if (graph_->removeEdge(source, destination))
    {
        sendResponse("Edge removed successfully.");
    }
    else
    {
        sendResponse("Failed to remove edge.");
    }
}

// Compute Minimum Spanning Tree
void ServerClient::computeMST()
{
    // Prompt the user to choose between Prim's or Kruskal's algorithm
    sendResponse("Choose MST algorithm (Prim/Kruskal): ");
    std::string algorithm = receiveChoice();

    // Create an MST object based on the user's choice using the factory pattern
    auto mst = MSTFactory::createMST(algorithm);

    // Compute the Minimum Spanning Tree using the chosen algorithm
    std::vector<Edge> mstEdges = mst->findMST(*graph_);

    // Prepare the response string with a formatted table of MST edges
    std::stringstream ss;
    ss << "\nMinimum Spanning Tree (" << algorithm << " algorithm):\n\n";
    ss << "+--------+--------+--------+\n";
    ss << "| Source | Dest   | Weight |\n";
    ss << "+--------+--------+--------+\n";

    // Iterate through the MST edges, format them into the table, and calculate total weight
    int totalWeight = 0;
    for (const auto &edge : mstEdges)
    {
        ss << "| " << std::setw(6) << edge.source
           << " | " << std::setw(6) << edge.destination
           << " | " << std::setw(6) << edge.weight << " |\n";
        totalWeight += edge.weight;
    }

    // Add the table footer and total MST weight
    ss << "+--------+--------+--------+\n";
    ss << "\nTotal MST Weight: " << totalWeight << "\n";

    // Send the formatted MST information back to the client
    sendResponse(ss.str());
}

// Handle MST queries
void ServerClient::handleMSTQueries()
{
    // Prompt the user to choose an MST algorithm (Prim or Kruskal)
    sendResponse("Choose MST algorithm (Prim/Kruskal): ");
    std::string algorithm = receiveChoice();

    // Create an MST object based on the chosen algorithm
    auto mst = MSTFactory::createMST(algorithm);

    // Compute the Minimum Spanning Tree edges using the chosen algorithm
    std::vector<Edge> mstEdges = mst->findMST(*graph_);

    // Create an MSTMetrics object to calculate various metrics
    MSTMetrics metrics;
    // Calculate metrics based on the graph and MST edges
    metrics.calculateMetrics(*graph_, mstEdges);

    // Prepare the response string with MST metrics
    std::stringstream ss;
    ss << "MST Metrics:\n";
    // Add the total weight of the MST
    ss << "Total Weight: " << MSTMetrics::getTotalWeight(mstEdges) << "\n";
    // Add the longest distance between any two vertices in the MST
    ss << "Longest Distance: " << MSTMetrics::getLongestDistance(*graph_, mstEdges) << "\n";
    // Add the average distance between vertices in the MST
    ss << "Average Distance: " << MSTMetrics::getAverageDistance(*graph_, mstEdges) << "\n";
    // Add the shortest distance (weight) of any edge in the MST
    ss << "Shortest Distance: " << MSTMetrics::getShortestDistance(mstEdges) << "\n";

    // Send the formatted response back to the client
    sendResponse(ss.str());
}

// Print the graph
void ServerClient::printGraph()
{
    sendResponse(graph_->toString());
}

// Main function
int main()
{
    // Create a ThreadPool with 4 worker threads
    ThreadPool pool(4);
    // Start the thread pool, which initializes the worker threads and starts the leader thread
    pool.start();

    // Print a message indicating the server has started
    safePrint("Server started. Type 'exit' to stop.");

    // Start a separate thread to handle user input for server shutdown
    std::thread inputThread([]()
                            {
        std::string input;
        while (running)
        {
            // Wait for user input
            std::getline(std::cin, input);
            // If the user types 'exit', set the running flag to false and break the loop
            if (input == "exit")
            {
                running = false;
                break;
            }
        } });

    // Main server loop
    while (running)
    {
        // Sleep for 100 milliseconds to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Print a message indicating the server is stopping
    safePrint("Stopping server...");
    // Stop the thread pool, which will clean up resources and join worker threads
    pool.stop();

    // If the input thread is still running, wait for it to finish
    if (inputThread.joinable())
    {
        inputThread.join();
    }

    // Exit the program
    return 0;
}
