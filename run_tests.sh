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
1
5
0 1 4
0 2 3
1 2 1
1 3 2
2 3 4
6
prim
7
6
kruskal
7
2
3
0 4 5
4
2
5
0 1
8
6
prim
7
1
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
6
kruskal
7
2
3
1 6 2
5
1 6
4
6
8
9
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