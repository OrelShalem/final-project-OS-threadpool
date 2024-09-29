#include "KruskalMST.hpp"
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <limits>
#include <vector>
#include <functional>

using namespace std;

vector<Edge> KruskalMST::findMST(const Graph &graph)
{
    if (graph.getVertices() < 2)
    {
        throw std::runtime_error("Graph must have at least 2 vertices for MST");
    }

    vector<Edge> mst;
    vector<Edge> allEdges;
    int numVertices = graph.getVertices();

    // Collect all edges from the graph
    for (int i = 0; i < numVertices; ++i)
    {
        vector<Edge> adjacentEdges = graph.getAdjacentEdges(i);
        allEdges.insert(allEdges.end(), adjacentEdges.begin(), adjacentEdges.end());
    }

    // Sort edges by weight
    sort(allEdges.begin(), allEdges.end());

    // Initialize disjoint set
    vector<int> parent(numVertices);
    for (int i = 0; i < numVertices; ++i)
    {
        parent[i] = i;
    }

    // Find function for disjoint set
    function<int(int)> find = [&](int v)
    {
        if (parent[v] != v)
        {
            parent[v] = find(parent[v]);
        }
        return parent[v];
    };

    // Union function for disjoint set
    auto unionSets = [&](int x, int y)
    {
        int rootX = find(x);
        int rootY = find(y);
        if (rootX != rootY)
        {
            parent[rootX] = rootY;
        }
    };

    // Kruskal's algorithm
    for (const Edge &edge : allEdges)
    {
        int sourceRoot = find(edge.source);
        int destRoot = find(edge.destination);

        if (sourceRoot != destRoot)
        {
            mst.push_back(edge);
            unionSets(sourceRoot, destRoot);
        }

        if (mst.size() == static_cast<size_t>(numVertices) - 1)
        {
            break;
        }
    }

    return mst;
}