#!/bin/bash

# Run server in background
./server &
SERVER_PID=$!

# Function to check if server is still running
is_server_running() {
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo "Server crashed unexpectedly"
        exit 1
    fi
}

# Wait for server to start
sleep 2

# Run client tests
./client << EOF
init 6
10
0 1 4
0 2 3
1 2 1
1 3 2
2 3 4
2 4 3
3 4 2
3 5 1
4 5 5
0 5 6
mst
kruskal
metric total_weight
metric longest_path
metric average_path
metric shortest_path
add_vtx
0
7
add_edge 6 1 8
add_edge 6 3 9
remove_edge 0 1
remove_vtx 2
mst
prim
metric total_weight
metric longest_path
metric average_path
metric shortest_path
init 3
3
0 1 1
1 2 2
0 2 3
mst
kruskal
add_edge 0 1 4
remove_edge 1 2
remove_vtx 1
add_edge 0 2 5
add_edge 0 1 6
change_weight 0 2 7
print_graph
exit
EOF

# Give the server some time to process the exit command
sleep 2

# Kill server if it's still running
if kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server is still running. Sending SIGTERM..."
    kill $SERVER_PID
    sleep 2
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo "Server did not respond to SIGTERM. Forcing shutdown with SIGKILL..."
        kill -9 $SERVER_PID
    fi
fi

wait $SERVER_PID 2>/dev/null

echo "Test run completed."