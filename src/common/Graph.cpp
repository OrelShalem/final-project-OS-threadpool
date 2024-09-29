#include "Graph.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
// מימוש הבנאי ברירת המחדל
Graph::Graph() : nextVertexId(0)
{
    // אין צורך לבצע פעולה מיוחדת כאן אם הגרף מתחיל ריק
    // ה-adjacencyList יישאר ריק עד שיוסיפו קשתות
}

Graph::Graph(int numVertices) : nextVertexId(numVertices)
{
    for (int i = 0; i < numVertices; ++i)
    {
        adjacencyList[i] = std::vector<Edge>();
    }
}

void Graph::addEdge(int source, int destination, int weight)
{
    std::cout << "Debug: Adding edge " << source << " - " << destination << " with weight " << weight << std::endl;
    adjacencyList[source].push_back(Edge(source, destination, weight));
    adjacencyList[destination].push_back(Edge(destination, source, weight));
}

int Graph::addVertex()
{
    int newVertexId = nextVertexId++;
    adjacencyList[newVertexId] = std::vector<Edge>();
    return newVertexId;
}

bool Graph::removeEdge(int source, int destination)
{
    auto &sourceEdges = adjacencyList[source];
    auto &destEdges = adjacencyList[destination];

    auto sourceIt = std::remove_if(sourceEdges.begin(), sourceEdges.end(),
                                   [destination](const Edge &e)
                                   { return e.destination == destination; });
    auto destIt = std::remove_if(destEdges.begin(), destEdges.end(),
                                 [source](const Edge &e)
                                 { return e.destination == source; });

    bool removed = sourceIt != sourceEdges.end() || destIt != destEdges.end();

    sourceEdges.erase(sourceIt, sourceEdges.end());
    destEdges.erase(destIt, destEdges.end());

    return removed;
}

bool Graph::removeVertex(int vertex)
{
    if (adjacencyList.find(vertex) == adjacencyList.end())
    {
        return false;
    }

    // הסר את כל הקשתות המחוברות לקודקוד זה
    for (auto &pair : adjacencyList)
    {
        pair.second.erase(
            std::remove_if(pair.second.begin(), pair.second.end(),
                           [vertex](const Edge &e)
                           { return e.destination == vertex; }),
            pair.second.end());
    }

    // הסר את הקודקוד עצמו
    adjacencyList.erase(vertex);

    // עדכן את מספרי הקודקודים האחרים אם צריך
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

bool Graph::changeWeight(int source, int destination, int newWeight)
{
    auto &sourceEdges = adjacencyList[source];
    auto &destEdges = adjacencyList[destination];

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

    sourceIt->weight = newWeight;
    destIt->weight = newWeight;

    return true;
}

std::vector<Edge> Graph::getAdjacentEdges(int vertex) const
{
    auto it = adjacencyList.find(vertex);
    if (it != adjacencyList.end())
    {
        return it->second;
    }
    return std::vector<Edge>();
}

int Graph::getVertices() const
{
    return adjacencyList.size();
}

int Graph::getEdges() const
{
    int count = 0;
    for (const auto &pair : adjacencyList)
    {
        count += pair.second.size();
    }
    return count / 2; // Each edge is counted twice
}

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