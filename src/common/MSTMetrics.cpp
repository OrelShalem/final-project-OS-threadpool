#include "MSTMetrics.hpp"
#include <limits>
#include <algorithm>
#include <numeric>
#include <vector>

using namespace std;

int MSTMetrics::getTotalWeight(const vector<Edge> &mst)
{
    int total = 0;
    for (const auto &edge : mst)
    {
        total += edge.weight;
    }
    return total;
}
//
int MSTMetrics::getLongestDistance(const Graph &graph, const vector<Edge> &mst)
{
    if (mst.empty())
    {
        return 0;
    }

    vector<vector<int>> dist(graph.getVertices(), vector<int>(graph.getVertices(), numeric_limits<int>::max()));

    // Initialize distances with MST edges
    for (const auto &edge : mst)
    {
        dist[edge.source][edge.destination] = edge.weight;
        dist[edge.destination][edge.source] = edge.weight;
    }

    // Floyd-Warshall algorithm to find all-pairs shortest paths
    for (int k = 0; k < graph.getVertices(); ++k)
    {
        for (int i = 0; i < graph.getVertices(); ++i)
        {
            for (int j = 0; j < graph.getVertices(); ++j)
            {
                if (dist[i][k] != numeric_limits<int>::max() &&
                    dist[k][j] != numeric_limits<int>::max() &&
                    dist[i][k] + dist[k][j] < dist[i][j])
                {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }

    // Find the maximum distance
    int maxDist = 0;
    for (int i = 0; i < graph.getVertices(); ++i)
    {
        for (int j = i + 1; j < graph.getVertices(); ++j)
        {
            if (dist[i][j] != numeric_limits<int>::max())
            {
                maxDist = max(maxDist, dist[i][j]);
            }
        }
    }

    return maxDist;
}

double MSTMetrics::getAverageDistance(const Graph &graph, const vector<Edge> &mst)
{
    if (mst.empty())
    {
        return 0.0;
    }

    int numVertices = graph.getVertices();
    vector<vector<int>> dist(numVertices, vector<int>(numVertices, numeric_limits<int>::max()));

    // Initialize distances with MST edges
    for (const auto &edge : mst)
    {
        dist[edge.source][edge.destination] = edge.weight;
        dist[edge.destination][edge.source] = edge.weight;
    }

    // Floyd-Warshall algorithm to find all-pairs shortest paths
    for (int k = 0; k < numVertices; ++k)
    {
        for (int i = 0; i < numVertices; ++i)
        {
            for (int j = 0; j < numVertices; ++j)
            {
                if (dist[i][k] != numeric_limits<int>::max() &&
                    dist[k][j] != numeric_limits<int>::max() &&
                    dist[i][k] + dist[k][j] < dist[i][j])
                {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }

    // Calculate the sum of all distances
    double totalDistance = 0.0;
    int validPairs = 0;
    for (int i = 0; i < numVertices; ++i)
    {
        for (int j = i + 1; j < numVertices; ++j)
        {
            if (dist[i][j] != numeric_limits<int>::max())
            {
                totalDistance += dist[i][j];
                validPairs++;
            }
        }
    }

    // Calculate and return the average distance
    return (validPairs > 0) ? (totalDistance / validPairs) : 0.0;
}

int MSTMetrics::getShortestDistance(const vector<Edge> &mst)
{
    if (mst.empty())
        return 0;

    size_t size = mst.size();
    vector<vector<int>> dist(size + 1, vector<int>(size + 1, numeric_limits<int>::max()));

    // Initialize distances with MST edges
    for (const auto &edge : mst)
    {
        dist[edge.source][edge.destination] = edge.weight;
        dist[edge.destination][edge.source] = edge.weight;
    }

    // Floyd-Warshall algorithm to find all-pairs shortest paths
    for (size_t k = 0; k <= size; ++k)
    {
        for (size_t i = 0; i <= size; ++i)
        {
            for (size_t j = 0; j <= size; ++j)
            {
                if (dist[i][k] != numeric_limits<int>::max() &&
                    dist[k][j] != numeric_limits<int>::max() &&
                    dist[i][k] + dist[k][j] < dist[i][j])
                {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }

    // Find the shortest distance between any two vertices
    int shortestDist = numeric_limits<int>::max();
    for (size_t i = 0; i <= size; ++i)
    {
        for (size_t j = i + 1; j <= size; ++j)
        {
            if (dist[i][j] != numeric_limits<int>::max())
            {
                shortestDist = min(shortestDist, dist[i][j]);
            }
        }
    }

    return shortestDist;
}

void MSTMetrics::calculateMetrics(const Graph &graph, const vector<Edge> &mst)
{
    totalWeight = getTotalWeight(mst);
    averageWeight = getAverageDistance(graph, mst);
    maxWeight = getLongestDistance(graph, mst);
    minWeight = getShortestDistance(mst);
}
