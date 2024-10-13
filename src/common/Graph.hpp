#pragma once
#include <unordered_map>
#include <vector>
#include <string>

struct Edge
{
    int source, destination, weight;
    Edge(int s, int d, int w) : source(s), destination(d), weight(w) {}

    bool operator<(const Edge &other) const
    {
        return weight < other.weight;
    }
};

class Graph
{
public:
    Graph();
    Graph(int numVertices);
    void addEdge(int source, int destination, int weight);
    int addVertex();
    bool removeEdge(int source, int destination);
    bool removeVertex(int vertex);
    bool changeWeight(int source, int destination, int newWeight);
    std::vector<Edge> getAdjacentEdges(int vertex) const;
    int getVertices() const;
    int getEdges() const;
    void printGraph() const;
    std::string toString() const;
    bool isConnected() const;
    bool isInitialized() const;
    void clear();

private:
    std::unordered_map<int, std::vector<Edge>> adjacencyList;
    static int nextVertexId;
};
