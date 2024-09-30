# Graph Class #
The Graph class represents an undirected graph using an adjacency list, where vertices are connected by edges with associated weights. Each edge is stored twice in the adjacency list (for both directions), and the class provides a variety of methods for managing the vertices and edges of the graph.

## Edge Struct ##
### Fields: ###
int source: The starting vertex of the edge.
int destination: The ending vertex of the edge.
int weight: The weight of the edge.
### operator<: ###
Defines comparison between two edges based on their weight, which is useful for sorting edges (e.g., in Kruskal's algorithm).

## Graph Constructors ##
### Graph(): ###
Default constructor that initializes an empty graph with no vertices.

### Graph(int numVertices): ###
Constructor that initializes a graph with a specified number of vertices, each with an empty adjacency list.

## Graph Methods ##
### void addEdge(int source, int destination, int weight): ###
Adds an undirected edge between two vertices with the given weight. The edge is added to the adjacency list for both vertices.

### int addVertex(): ###
Adds a new vertex to the graph and returns its ID. Vertices are assigned incremental IDs starting from 0.

### bool removeEdge(int source, int destination): ###
Removes the edge between the specified vertices, if it exists, from both directions in the adjacency list. Returns true if the edge was successfully removed, false otherwise.

### bool removeVertex(int vertex): ###
Removes the specified vertex and all edges connected to it. Additionally, it updates the IDs of vertices with higher numbers, reducing each by 1.

### bool changeWeight(int source, int destination, int newWeight): ###
Changes the weight of the edge between two vertices, if the edge exists. Returns true if the weight was successfully updated.

### vector<Edge> getAdjacentEdges(int vertex) const: ###
Returns a vector containing all the edges adjacent to the given vertex.

### int getVertices() const: ###
Returns the total number of vertices currently in the graph.

### int getEdges() const: ###
Returns the total number of edges in the graph. Since each edge is stored twice (once for each direction), the count is divided by 2.

### void printGraph() const: ###
Prints a textual representation of the graph to the console, showing each vertex and its adjacent edges.

### string toString() const: ###
Returns a string representation of the graph, including the number of vertices and details of all edges.

#####################################################################################

# KruskalMST Class #
The KruskalMST class implements Kruskal's algorithm to find the Minimum Spanning Tree (MST) of an undirected, weighted graph. It inherits from the MST base class and overrides the findMST method.

## KruskalMST Methods ##
### vector<Edge> findMST(const Graph &graph) override: ###
This method uses Kruskal's algorithm to compute the MST of the given graph. It first collects all edges, sorts them by weight, and uses a disjoint-set data structure to avoid cycles. Edges are added to the MST until the tree contains V - 1 edges (where V is the number of vertices).

#####################################################################################

# PrimMST Class #
The PrimMST class implements Prim's algorithm to find the Minimum Spanning Tree (MST) of an undirected, weighted graph. It inherits from the MST base class and overrides the findMST method.

## PrimMST Methods ##
### vector<Edge> findMST(const Graph &graph) override: ###
This method uses Prim's algorithm to compute the MST. It starts from an arbitrary vertex, using a priority queue to select the edge with the smallest weight that connects an unvisited vertex. The algorithm continues until all vertices are included in the MST.

#####################################################################################

# MST Class #
The MST class is an abstract base class that defines the interface for finding the Minimum Spanning Tree (MST) of a graph. It serves as a blueprint for concrete implementations of MST algorithms like Kruskal and Prim.

## MST Methods ##
### virtual vector<Edge> findMST(const Graph &graph) = 0: ###
A pure virtual function that must be implemented by derived classes. It computes and returns the MST of the given graph as a vector of edges.

#####################################################################################

# MSTFactory Class #
The MSTFactory class provides a factory method to create instances of different Minimum Spanning Tree (MST) algorithms (Prim's and Kruskal's). It allows clients to choose an MST algorithm dynamically at runtime.

## MSTFactory Methods ##
### static unique_ptr<MST> createMST(const string &algorithm): ###
This static method takes a string representing the desired algorithm ("prim" or "kruskal") and returns a unique pointer to the corresponding MST object. If the algorithm name is not recognized, it throws an invalid_argument exception.

#####################################################################################

# MSTMetrics Class #
The MSTMetrics class provides various metrics for the Minimum Spanning Tree (MST) of a graph, such as total weight, average distance, longest distance, and shortest distance. It allows for comprehensive analysis of the MST results using the Floyd-Warshall algorithm to compute shortest paths between all pairs of vertices.

## MSTMetrics Methods ##
### static int getTotalWeight(const vector<Edge> &mst): ###
Returns the total weight of all edges in the MST by summing up the weights of the edges.

###static int getLongestDistance(const Graph &graph, const vector<Edge> &mst): ###
Calculates the longest distance between any two vertices in the MST using the Floyd-Warshall algorithm to find all-pairs shortest paths.

### static double getAverageDistance(const Graph &graph, const vector<Edge> &mst): ###
Computes the average distance between all pairs of vertices in the MST, utilizing the Floyd-Warshall algorithm to calculate shortest paths.

### static int getShortestDistance(const vector<Edge> &mst): ###
Finds the shortest distance between any two vertices in the MST.

### void calculateMetrics(const Graph &graph, const vector<Edge> &mst): ###
Calculates all the MST metrics (total weight, average weight, longest distance, and shortest distance) and stores the results within the class object.

#####################################################################################