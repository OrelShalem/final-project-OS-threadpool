#include "PrimMST.hpp"
#include <queue>
#include <unordered_set>
#include <limits>
#include <stdexcept>

using namespace std;

std::vector<Edge> PrimMST::findMST(const Graph &graph)
{
    if (graph.getVertices() < 2)
    {
        throw std::runtime_error("Graph must have at least 2 vertices for MST");
    }

    vector<Edge> mst;
    int n = graph.getVertices();
    vector<bool> visited(n, false);
    vector<int> key(n, numeric_limits<int>::max());
    vector<int> parent(n, -1);

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;

    int startVertex = 0;
    pq.push({0, startVertex});
    key[startVertex] = 0;

    while (!pq.empty())
    {
        int u = pq.top().second;
        pq.pop();

        if (visited[u])
            continue;

        visited[u] = true;

        if (parent[u] != -1)
        {
            mst.push_back({parent[u], u, key[u]});
        }

        for (const auto &neighbor : graph.getAdjacentEdges(u))
        {
            int v = neighbor.destination;
            int weight = neighbor.weight;

            if (!visited[v] && weight < key[v])
            {
                parent[v] = u;
                key[v] = weight;
                pq.push({key[v], v});
            }
        }
    }

    return mst;
}