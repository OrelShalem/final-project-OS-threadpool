#pragma once
#include <unordered_map>
#include <vector>
#include <string> // הוסף שורה זו

struct Edge
{
    int source, destination, weight;
    Edge(int s, int d, int w) : source(s), destination(d), weight(w) {}

    // Add this operator
    bool operator<(const Edge &other) const
    {
        return weight < other.weight;
    }
};

class Graph
{
public:
    Graph(); // בנאי ברירת מחדל
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

private:
    std::unordered_map<int, std::vector<Edge>> adjacencyList;
    int nextVertexId;
};