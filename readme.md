# Minimum Spanning Tree (MST) Server-Client Project

## Overview
This project implements a server-client architecture for managing and analyzing graphs, with a focus on Minimum Spanning Tree (MST) algorithms. The server can handle multiple clients concurrently using a thread pool, and supports various graph operations and MST computations.

## Key Components

### Graph Class (src/common/Graph.hpp, src/common/Graph.cpp)
- Represents an undirected weighted graph using an adjacency list.
- Supports operations like adding/removing vertices and edges, changing edge weights, and querying graph properties.
- Implements thread-safe operations to ensure data integrity in a multi-threaded environment.

### MST Algorithms
- Implements both Kruskal's and Prim's algorithms for finding the Minimum Spanning Tree.
- Uses a factory pattern (MSTFactory) to create the appropriate MST algorithm instance.
- Allows for easy extension to include additional MST algorithms in the future.

### MSTMetrics Class
- Provides various metrics for analyzing the Minimum Spanning Tree, including total weight, longest distance, average distance, and shortest distance.
- Utilizes efficient algorithms to compute these metrics, ensuring scalability for large graphs.

### ThreadPool Class (src/utils/threadpool.hpp, src/utils/threadpool.cpp)
- Manages a pool of worker threads to handle client connections concurrently.
- Implements a leader-follower pattern for accepting new connections and delegating client handling.
- Ensures efficient utilization of system resources and improved server performance.

### Server (src/server/server.hpp, src/server/server.cpp)
- Listens for client connections and manages the overall server operation.
- Uses the ThreadPool to handle multiple clients simultaneously.
- Implements a robust command processing system to interpret and execute client requests.

### Client (src/client/client.cpp)
- Connects to the server and allows users to interact with the graph and perform MST operations.
- Provides a user-friendly interface for sending commands and receiving responses.

## Key Features
1. Graph Construction and Modification
   - Dynamic graph creation with support for adding and removing vertices and edges.
   - Weight modification for existing edges.
2. MST Computation (Kruskal's and Prim's algorithms)
   - On-demand calculation of Minimum Spanning Trees using either Kruskal's or Prim's algorithm.
3. MST Metrics Calculation
   - Comprehensive analysis of MST properties including total weight and various distance metrics.
4. Concurrent Client Handling
   - Efficient management of multiple client connections using a thread pool.
5. Thread-Safe Graph Operations
   - Ensures data integrity when multiple clients are modifying the graph simultaneously.

### Building the Project

1. Clone the repository:
   ```
   git clone [your-repository-url]
   cd [repository-name]
   ```

2. Build the project using Make:
   ```
   make all
   ```
   This command will compile both the server and client executables.

### Running the Server

1. Start the server:
   ```
   ./server
   ```
   The server will start and listen on port 9039 by default.

2. To stop the server, type `exit` in the server console or use Ctrl+C.

### Running the Client

1. In a new terminal window, start the client:
   ```
   ./client
   ```
   The client will attempt to connect to the server running on localhost:9039.

2. Follow the on-screen prompts to interact with the server.

3. To exit the client, type `exit` or `9` when prompted for a command.

### Running Tests

To run the test suite and generate a coverage report: