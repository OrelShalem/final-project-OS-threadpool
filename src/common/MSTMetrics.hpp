#pragma once
#include <vector>
#include "Graph.hpp"
#include "MST.hpp"

using namespace std;

class MSTMetrics
{
public:
    // get the total weight of the MST
    static int getTotalWeight(const vector<Edge> &mst);
    // get the longest distance in the MST
    static int getLongestDistance(const Graph &graph, const vector<Edge> &mst);
    // get the average distance in the MST
    static double getAverageDistance(const Graph &graph, const vector<Edge> &mst);
    // get the shortest distance in the MST
    static int getShortestDistance(const vector<Edge> &mst);
    // calculate the metrics of the MST
    void calculateMetrics(const Graph &graph, const vector<Edge> &mst);

private:
    double totalWeight;
    double averageWeight;
    double maxWeight;
    double minWeight;
};