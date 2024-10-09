##################################################################################
COMMON FILES (server and client)
##################################################################################

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

##################################################################################

# KruskalMST Class #
The KruskalMST class implements Kruskal's algorithm to find the Minimum Spanning Tree (MST) of an undirected, weighted graph. It inherits from the MST base class and overrides the findMST method.

## KruskalMST Methods ##
### vector<Edge> findMST(const Graph &graph) override: ###
This method uses Kruskal's algorithm to compute the MST of the given graph. It first collects all edges, sorts them by weight, and uses a disjoint-set data structure to avoid cycles. Edges are added to the MST until the tree contains V - 1 edges (where V is the number of vertices).

##################################################################################

# PrimMST Class #
The PrimMST class implements Prim's algorithm to find the Minimum Spanning Tree (MST) of an undirected, weighted graph. It inherits from the MST base class and overrides the findMST method.

## PrimMST Methods ##
### vector<Edge> findMST(const Graph &graph) override: ###
This method uses Prim's algorithm to compute the MST. It starts from an arbitrary vertex, using a priority queue to select the edge with the smallest weight that connects an unvisited vertex. The algorithm continues until all vertices are included in the MST.

##################################################################################

# MST Class #
The MST class is an abstract base class that defines the interface for finding the Minimum Spanning Tree (MST) of a graph. It serves as a blueprint for concrete implementations of MST algorithms like Kruskal and Prim.

## MST Methods ##
### virtual vector<Edge> findMST(const Graph &graph) = 0: ###
A pure virtual function that must be implemented by derived classes. It computes and returns the MST of the given graph as a vector of edges.

##################################################################################

# MSTFactory Class #
The MSTFactory class provides a factory method to create instances of different Minimum Spanning Tree (MST) algorithms (Prim's and Kruskal's). It allows clients to choose an MST algorithm dynamically at runtime.

## MSTFactory Methods ##
### static unique_ptr<MST> createMST(const string &algorithm): ###
This static method takes a string representing the desired algorithm ("prim" or "kruskal") and returns a unique pointer to the corresponding MST object. If the algorithm name is not recognized, it throws an invalid_argument exception.

##################################################################################

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

##################################################################################
CLIENT
##################################################################################

# Client Class #
The Client class handles communication between the client and the server. It manages the connection, sending requests, and receiving responses from the server. It also handles user interaction for inputting commands and processing responses.

## Client Methods ##
### Client(const std::string &address, int port) ###
This constructor initializes a Client object with the server's address and port number.

#### Details: ####
It initializes the sock member variable to -1, indicating that the client socket has not yet been created.

### ~Client() ###
This destructor ensures that the socket is properly closed when the Client object is destroyed.

#### Details: ####
If the socket is open (sock != -1), it is closed using the close() function, preventing any resource leakage.

### void connectToServer() ###
Establishes a connection to the server using the provided address and port number.

#### Details: ####
This method creates a socket (AF_INET for IPv4, SOCK_STREAM for TCP), and attempts to connect to the server's address. If the connection is successful, it receives and displays the initial menu from the server. If any errors occur during socket creation or connection, appropriate error messages are printed.

### void sendRequest(const std::string &request) ###
Sends a request string to the server and processes the response.

#### Details: ####
The function ensures the client is connected (connectToServer() is called), then sends the request to the server. Based on the server's response, the client may prompt the user for additional input, such as edge details when creating a graph or algorithm choices for calculating the MST. If the server is busy (e.g., when the graph is in use), the client offers the option to retry.

### void sendMetricsRequests() ###
Sends a series of requests to the server to compute and display additional MST metrics.

#### Details: ####
This method iterates over a list of predefined metrics (total_weight, longest_path, average_path, and shortest_path), sending each metric request to the server. It then processes and prints the server's response for each metric.

### string receiveResponse() ###
Receives the response from the server and returns it as a string.

#### Details: ####
This function uses a timeout mechanism (select()) to wait for data to be available on the socket. It reads chunks of data from the server, appending each chunk to the fullResponse string, until either a timeout occurs or the server stops sending data. This approach ensures that partial or delayed responses are handled gracefully.

##################################################################################
SERVER
##################################################################################

# Server Class #
The Server class manages the main server functionality, including accepting client connections, handling graph-related operations, and managing a shared graph resource. It utilizes a thread pool to process multiple client requests concurrently, ensuring efficient use of system resources.

## Server Methods ##
### Server(int port) ###
The constructor initializes the server by creating a socket, setting socket options for reuse, and binding the server to the specified port. It also sets up the server to listen for incoming client connections and initializes the shared graph and the thread pool with four threads.

### ~Server() ###
The destructor cleans up resources by closing the server socket, ensuring a proper shutdown of the server.

### void run() ###
This method contains the main server loop, which continuously listens for client connections while the server is running. It uses select to monitor the server socket for incoming connections and handles multiple clients using a non-blocking approach. When a client connects, the connection is handled in a separate thread from the thread pool. The loop exits when the server is set to shut down.

### void shutdown() ###
This method initiates the shutdown process for the server. It sets the shouldExit flag to true, which signals the server to stop accepting new connections. It prints a message indicating that the server is shutting down and closes the server socket if it's still open. Finally, it instructs the thread pool to exit, ensuring all threads complete their tasks before the server terminates.

### void handleClient(int clientSocket) ###
This method manages communication with a connected client. It begins by sending the client a menu of available commands. The method then enters a loop where it listens for client requests. Upon receiving a request, it processes it, handling various commands such as initializing the graph, adding or removing vertices and edges, calculating the Minimum Spanning Tree (MST), and printing the graph. The method also includes debugging statements to log received commands and any errors encountered. If the client disconnects or an error occurs during communication, the loop exits and the client socket is closed.

### void handleInitCommand(int clientSocket, std::istringstream &iss) ###
This method processes the "init" command from a client, which initializes the shared graph. It first attempts to acquire a lock on the graph to ensure thread safety. If successful, it reads the number of vertices for the graph. If the number of vertices is valid, it initializes the sharedGraph. The method then waits for a command from the client to specify the number of edges and reads edge definitions from the client, adding them to the graph as they are received. It sends responses back to the client to confirm the actions taken

### void handleAddVertexCommand(int clientSocket, std::istringstream &iss) ###
This method processes the "add_vtx" command to add a new vertex to the graph. It first attempts to acquire a lock on the graph to ensure thread safety. If successful, it retrieves the current number of vertices and adds a new vertex to the sharedGraph. The method then prompts the client for an existing vertex index to connect to and reads the input. If the input is valid, it requests the weight for the edge to be added. After reading the weight, it adds the edge between the new vertex and the existing vertex. The method sends confirmation messages to the client regarding the actions taken and provides an updated representation of the graph. Finally, it releases the graph lock.

### void handleAddEdgeCommand(int clientSocket, std::istringstream &iss) ###
This method processes the "add_edge" command, allowing the client to add an edge between two existing vertices. It begins by acquiring a lock on the graph. The method prompts the client for the source, destination, and weight of the edge and reads these values. If the provided vertex indices are valid, it adds the edge to the sharedGraph. The method sends a confirmation message to the client and provides an updated graph representation. If the vertex indices are invalid, an error message is sent. Finally, it releases the graph lock.

### void handleRemoveEdgeCommand(int clientSocket, std::istringstream &iss) ###
This method handles the "remove_edge" command, allowing the client to remove an edge between two vertices. It first acquires a lock on the graph. After prompting the client for the source and destination of the edge to be removed, it reads these values. If the vertex indices are valid, it attempts to remove the edge from the sharedGraph. A confirmation or error message is sent to the client depending on whether the edge existed. The method also provides an updated graph representation before releasing the graph lock.

### void handleRemoveVertexCommand(int clientSocket, std::istringstream &iss) ###
This method processes the "remove_vtx" command, allowing the client to remove a vertex from the graph. It first attempts to acquire a lock on the graph for thread safety. The method prompts the client to input the index of the vertex to be removed. If the input is invalid (either non-integer or out of range), it sends an error message to the client. If the input is valid, it calls sharedGraph.removeVertex(vertexToRemove) to remove the specified vertex along with any edges connected to it. After successfully removing the vertex, it sends a confirmation message and provides an updated representation of the graph before releasing the lock.

### void handleMSTCommand(int clientSocket) ###
This method handles the "mst" command, which calculates the Minimum Spanning Tree (MST) for the graph. It begins by acquiring a lock on the graph. If the graph is empty or has no edges, it sends an error message and releases the lock. The method then prompts the client to choose between Prim's or Kruskal's algorithm to compute the MST. After reading the client's choice, it validates the input. If the input is invalid, an error message is sent.

If valid, the method attempts to create an MST algorithm instance using the MSTFactory::createMST(mstType) method and calls findMST(sharedGraph) to compute the MST. If successful, the result is stored in lastMST. In case of any exceptions, an error message is sent to the client.

To provide a clear representation of the MST, the method builds a tree structure using an unordered map. It defines a recursive helper function printTree to format the tree for display. This function uses a prefix to visualize the tree's structure. After printing the tree starting from the first node in the MST, the formatted string is sent to the client, and the graph lock is released.

### void handleMetricCommand(int clientSocket, std::istringstream &iss) ###
This method processes the "metric" command, allowing the client to request various metrics related to the previously calculated Minimum Spanning Tree (MST). It first checks if the graph is initialized and if an MST has been computed. If not, appropriate error messages are sent to the client.

The method then reads the metric type from the input stream and checks which metric the client requested:

#### Total Weight: It calculates the total weight of the MST using
MSTMetrics::getTotalWeight(lastMST) and constructs a response string.

#### Longest Path: It retrieves the longest path in the MST using
MSTMetrics::getLongestDistance(sharedGraph, lastMST).

#### Average Path: It calculates the average path length using
MSTMetrics::getAverageDistance(sharedGraph, lastMST).

#### Shortest Path: It finds the shortest path using
MSTMetrics::getShortestDistance(lastMST).

If the metric type is unknown, it sends a corresponding error message. Finally, it sends the computed response back to the client and releases the graph lock.

### void handleChangeWeightCommand(int clientSocket, std::istringstream &iss) ###
This method handles the "change_weight" command, allowing the client to modify the weight of an edge between two vertices in the graph. It begins by acquiring a lock on the graph for safety. The method expects three integers as input: the source vertex, destination vertex, and the new weight. If the input is invalid, it sends an error message.

If the edge exists and the weight is successfully changed via sharedGraph.changeWeight(source, destination, newWeight), it confirms the change to the client. If the edge is not found or if the vertices are invalid, it sends an appropriate error message. Finally, the graph lock is released.

### void handlePrintGraphCommand(int clientSocket) ###
This method processes the "print_graph" command, which sends the current representation of the graph to the client. It acquires a lock on the graph and uses sharedGraph.toString() to obtain the graph's string representation. This representation is then sent back to the client, followed by releasing the graph lock.

### void sendResponse(int clientSocket, const std::string &message) ###
This helper method sends a message back to the client and also prints the message to the console. It locks a mutex to ensure thread safety while writing to the console. The message is sent using write(clientSocket, message.c_str(), message.size()).

### void sendMenu(int clientSocket) ###
This method constructs and sends a menu of available commands to the client. It builds a string stream containing the menu options and calls sendResponse(clientSocket, menuStream.str()) to send the menu.

### bool acquireGraphLock(int clientSocket) ###
This method attempts to acquire a lock on the shared graph. It uses a unique lock to ensure that only one client can access the graph at a time. If the graph is already in use (isGraphBusy is true), it sends an error message to the client and returns false. If successful, it sets isGraphBusy to true and returns true.

### void releaseGraphLock() ###
This method releases the lock on the shared graph. It locks the mutex and sets isGraphBusy to false, allowing other clients to access the graph. It also notifies one waiting client (if any) that the graph is now available.

##################################################################################
THREADPOOL
##################################################################################

# ThreadPool Class #
The ThreadPool class manages a pool of worker threads that can asynchronously execute tasks from a queue. This allows tasks to be processed in parallel, improving performance in multi-threaded applications. The class uses synchronization mechanisms to ensure safe access to the shared queue of tasks and manages the lifecycle of the worker threads.

### ThreadPool(size_t threads) ###
Initializes the thread pool with a specified number of worker threads (threads).
Each thread is created and started immediately, running the workerThread() function.
The stop flag is set to false, indicating that the thread pool is ready to accept and process tasks.

#### Key Elements: ####
workers: A vector that holds the actual threads.
A lambda function inside the constructor spawns each thread and calls workerThread(), which runs continuously until the pool is stopped.

### ~ThreadPool() ###
Cleans up the thread pool when it goes out of scope or is explicitly deleted.
Calls the exit() function to ensure that all threads are properly stopped and joined before the pool is destroyed.

### void enqueue(function<void()> task) ###
Adds a new task to the queue, which will later be picked up and executed by one of the worker threads.

#### Key Details: ####
Locks the queue with queueMutex to ensure only one thread can add a task at a time, preventing race conditions.
Pushes the task (a function or lambda) into the tasks queue.
Notifies one of the worker threads (waiting on the condition variable condition) that there is a new task to process.

### void workerThread() ###
This is the main function that each worker thread runs. It continuously loops, waiting for tasks to be added to the queue, and then executes them.
The loop exits when the stop flag is set and the task queue is empty.

#### Key Details: ####
Each thread waits on the condition variable condition until either there is a task available (!tasks.empty()) or the pool is stopping (stop is true).
When a task is available, it is removed from the queue, and the thread executes it.
If the thread pool is stopping and there are no tasks left, the thread exits.

### void exit() ###
Safely shuts down the thread pool by setting the stop flag and ensuring that all worker threads finish their tasks and then stop.
Notifies all threads that they should stop waiting and finish execution.

#### Key Details: ####
Locks the queueMutex to safely modify the stop flag.
The condition.notify_all() ensures that all threads wake up (if they are waiting) and check the stop flag to exit gracefully.
Each thread is joined using worker.join() to make sure the main thread waits for all worker threads to finish before destroying the pool.

### Private Data Members ###
#### vector<thread> workers: ####
Stores all the worker threads.
Each thread runs the workerThread() function and remains active until the thread pool is stopped.

#### queue<function<void()>> tasks: ####
A queue that stores tasks as std::function<void()>, allowing the pool to handle any callable tasks (e.g., functions, lambdas).

#### mutex queueMutex: ####
Ensures that access to the tasks queue is thread-safe, preventing race conditions when tasks are added or removed.

#### condition_variable condition: ####
Used to block the worker threads while waiting for new tasks or for the pool to be stopped. When a task is added, the condition variable notifies one of the threads to wake up and handle the task.

#### atomic<bool> stop: ####
An atomic boolean flag that signals when the thread pool should stop accepting tasks and terminate the worker threads. Atomicity ensures that the stop flag is updated and read in a thread-safe manner

##################################################################################
Executable files (server and clients)
##################################################################################

# Server Code (main.cpp) #S
This is the entry point for running the server. It handles setting up the server, managing console input, and gracefully shutting down the server when requested.

### Server *globalServerPtr ###
This pointer holds a reference to the running Server instance so that the signal handler can safely shut down the server when a termination signal is received.

### Signal Handler Function: signalHandler(int signum) ###
Catches system signals (SIGINT, SIGTERM) such as when the user presses Ctrl+C to stop the server.
If the server is running (globalServerPtr is not nullptr), the server's shutdown() function is called to gracefully shut it down before exiting the program.

### Console Input Thread: void consoleInputThread(Server &server) ###
Runs in a separate thread to monitor input from the console.
Listens for the exit command from the user to manually shut down the server.
When the exit command is entered, the server shuts down gracefully, and the thread exits.

#### Key Details: ####
It uses std::getline() to read input and checks if the command is "exit".
Calls Server::lockCout() and Server::unlockCout() to ensure that console output is synchronized when printing messages.
Once the exit command is received, it calls the server.shutdown() method to stop the server, and the loop terminates.

### int main() ###
Initializes and runs the Server.
Handles signal registration and starts a thread to monitor user input for shutdown commands.

#### Key Details: ####
The Server object is instantiated with port 9036.
The globalServerPtr is set to the address of the Server instance to allow the signal handler to access it.
Signal Handling: The program registers signalHandler for handling SIGINT and SIGTERM, allowing graceful shutdown upon system signals.
A separate thread (inputThread) is started for reading console input using the consoleInputThread() function. This thread runs concurrently with the server to allow for graceful shutdown via console commands.
The server.run() function starts the server, allowing it to handle client requests.
The inputThread is joined using inputThread.join() to ensure the main thread waits for the console input thread to finish before the program exits.

After the server shuts down, globalServerPtr is reset to nullptr.

# Client Code (main.cpp) #
This is the entry point for the client application. It handles connecting to the server, sending commands, and receiving responses from the server.

### int main() ###
Manages client-side operations such as connecting to the server, sending commands, and handling server responses.

#### Key Details: ####

The client is instantiated with the server IP (127.0.0.1) and port (9036).
The client.connectToServer() function connects to the server and triggers the initial menu display.
A while (true) loop is used to continuously read user input from the console. The input is read using std::getline() and stored in the command string.

#### Command Handling: ####
If the user enters "exit", the client sends the exit command to the server, breaking the loop, and exits the client.
For any other input, the command is sent to the server using client.sendRequest(command + "\n"). The server processes the command and sends back a response.
The client application exits after sending the exit command or upon terminating the loop.

### Explanation of Key Functions ###

#### Server ####
server.run(): Starts the server, allowing it to listen for client connections and process incoming requests.
server.shutdown(): Gracefully shuts down the server by stopping it from accepting new connections and ensuring all active connections are handled.
Server::lockCout() / Server::unlockCout(): These functions lock and unlock the console output, ensuring that multiple threads do not print to the console simultaneously, which could lead to jumbled messages.

#### Client ####
client.connectToServer(): Establishes a connection to the server at the specified IP and port. This function also receives and prints the server's initial menu.
client.sendRequest(): Sends the client's command or request to the server. The server processes the command and responds accordingly.

##################################################################################
HOW TO RUN THE PROGRAM
##################################################################################

## run server and client (from two different terminal, after we do make all): ##
./server 
./client

## run memcheck (from two different terminal): ##
make memcheck_server
make memcheck_client

## run helgrind (from two different terminal): ##
make helgrind_server
make helgrind_client

## run codecoverage (it will run the script) ##
make coverage


##################################################################################
FLOW OF THE PROGRAM
##################################################################################
+------------------+
|      Server      |
+------------------+
         |
         | Creates ThreadPool
         v
+------------------+
|    ThreadPool    |
+------------------+
    |    |    |    |
    |    |    |    |
    v    v    v    v
+----+ +----+ +----+ +----+
| W1 | | W2 | | W3 | | W4 | (Worker Threads)
+----+ +----+ +----+ +----+
    ^    ^    ^    ^
    |    |    |    |
    |    |    |    |
+------------------+
|    Task Queue    |
+------------------+
         ^
         |
         | Enqueues tasks
+------------------+
| Client Connections|
+------------------+