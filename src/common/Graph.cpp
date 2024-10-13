#include "Graph.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include "../utils/threadpool.hpp"

// This file implements the Graph class, which represents an undirected weighted graph.

int Graph::nextVertexId = 0;

// Default constructor: Initializes an empty graph
Graph::Graph()
{
    // No special action needed here as the graph starts empty
    // The adjacencyList will remain empty until edges are added
}

// Constructor with a specified number of vertices
Graph::Graph(int numVertices)
{
    // Initialize the graph with 'numVertices' vertices, each with an empty edge list
    for (int i = 0; i < numVertices; ++i)
    {
        adjacencyList[i] = std::vector<Edge>();
    }
}

// Adds an edge between two vertices with a specified weight
void Graph::addEdge(int source, int destination, int weight)
{
    safePrint("Debug: Adding edge " + std::to_string(source) + " - " + std::to_string(destination) + " with weight " + std::to_string(weight));
    // Add the edge in both directions (undirected graph)
    adjacencyList[source].push_back(Edge(source, destination, weight));
    adjacencyList[destination].push_back(Edge(destination, source, weight));
}

// Adds a new vertex to the graph and returns its ID
int Graph::addVertex()
{
    int newVertexId = nextVertexId++;
    adjacencyList[newVertexId] = std::vector<Edge>();
    return newVertexId;
}

// Removes an edge between two vertices if it exists
bool Graph::removeEdge(int source, int destination)
{
    auto &sourceEdges = adjacencyList[source];
    auto &destEdges = adjacencyList[destination];

    // Remove the edge from source to destination
    auto sourceIt = std::remove_if(sourceEdges.begin(), sourceEdges.end(),
                                   [destination](const Edge &e)
                                   { return e.destination == destination; });
    // Remove the edge from destination to source
    auto destIt = std::remove_if(destEdges.begin(), destEdges.end(),
                                 [source](const Edge &e)
                                 { return e.destination == source; });

    bool removed = sourceIt != sourceEdges.end() || destIt != destEdges.end();

    // Erase the removed edges
    sourceEdges.erase(sourceIt, sourceEdges.end());
    destEdges.erase(destIt, destEdges.end());

    return removed;
}

// Removes a vertex and all its connected edges from the graph
bool Graph::removeVertex(int vertex)
{
    if (adjacencyList.find(vertex) == adjacencyList.end())
    {
        return false;
    }

    // Remove all edges connected to this vertex
    for (auto &pair : adjacencyList)
    {
        pair.second.erase(
            std::remove_if(pair.second.begin(), pair.second.end(),
                           [vertex](const Edge &e)
                           { return e.destination == vertex; }),
            pair.second.end());
    }

    // Remove the vertex itself
    adjacencyList.erase(vertex);

    // Update other vertex numbers if necessary
    std::unordered_map<int, std::vector<Edge>> newAdjacencyList;
    for (const auto &pair : adjacencyList)
    {
        int newVertex = pair.first > vertex ? pair.first - 1 : pair.first;
        for (const Edge &e : pair.second)
        {
            int newDest = e.destination > vertex ? e.destination - 1 : e.destination;
            newAdjacencyList[newVertex].push_back({newVertex, newDest, e.weight});
        }
    }
    adjacencyList = std::move(newAdjacencyList);

    return true;
}

// Changes the weight of an edge between two vertices
bool Graph::changeWeight(int source, int destination, int newWeight)
{
    auto &sourceEdges = adjacencyList[source];
    auto &destEdges = adjacencyList[destination];

    // Find the edge in both directions
    auto sourceIt = std::find_if(sourceEdges.begin(), sourceEdges.end(),
                                 [destination](const Edge &e)
                                 { return e.destination == destination; });
    auto destIt = std::find_if(destEdges.begin(), destEdges.end(),
                               [source](const Edge &e)
                               { return e.destination == source; });

    if (sourceIt == sourceEdges.end() || destIt == destEdges.end())
    {
        return false;
    }

    // Update the weight in both directions
    sourceIt->weight = newWeight;
    destIt->weight = newWeight;

    return true;
}

// Returns a vector of edges adjacent to a given vertex
std::vector<Edge> Graph::getAdjacentEdges(int vertex) const
{
    auto it = adjacencyList.find(vertex);
    if (it != adjacencyList.end())
    {
        return it->second;
    }
    return std::vector<Edge>();
}

// Returns the number of vertices in the graph
int Graph::getVertices() const
{
    return adjacencyList.size();
}

// Returns the number of edges in the graph
int Graph::getEdges() const
{
    int count = 0;
    for (const auto &pair : adjacencyList)
    {
        count += pair.second.size();
    }
    return count / 2; // Each edge is counted twice in an undirected graph
}

// Prints the graph structure to the console
void Graph::printGraph() const
{
    for (const auto &pair : adjacencyList)
    {
        std::cout << pair.first << ": ";
        for (const auto &edge : pair.second)
        {
            std::cout << "(" << edge.source << ", " << edge.destination << ", " << edge.weight << ") ";
        }
        std::cout << std::endl;
    }
}

// Returns a string representation of the graph
std::string Graph::toString() const
{
    std::ostringstream oss;
    oss << "Debug: Graph has " << adjacencyList.size() << " vertices.\n";
    for (const auto &pair : adjacencyList)
    {
        oss << "Vertex " << pair.first << ":\n";
        oss << "Debug: This vertex has " << pair.second.size() << " edges.\n";
        if (pair.second.empty())
        {
            oss << "  (no edges)\n";
        }
        else
        {
            for (const auto &edge : pair.second)
            {
                oss << "  -> " << edge.destination << " (weight: " << edge.weight << ")\n";
            }
        }
    }
    return oss.str();
}

// Checks if the graph is connected (all vertices are reachable from any other vertex)
bool Graph::isConnected() const
{
    if (adjacencyList.empty())
    {
        return false; // An empty graph is not considered connected
    }

    // Note: This implementation is incorrect and always returns true
    // A proper implementation would use a graph traversal algorithm (e.g., DFS or BFS)
    // to check if all vertices are reachable from a single starting point
    return true;
}

// Checks if the graph has been initialized with any vertices
bool Graph::isInitialized() const
{
    return !adjacencyList.empty(); // Returns true if the adjacency list is not empty
}

// Clears all vertices and edges from the graph
void Graph::clear()
{
    adjacencyList.clear(); // Removes all entries from the adjacency list
    // The comment below is in Hebrew and states:
    // "There's no need to reset nextVertexId here"
    // אין צורך לאפס את nextVertexId כאן
}
