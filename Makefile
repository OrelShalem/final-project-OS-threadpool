CXX = g++
CXXFLAGS = -std=c++14 -pthread -g

SRCS = src/main.cpp \
       src/server/Server.cpp \
       src/utils/ThreadPool.cpp \
       src/common/Graph.cpp \
       src/common/KruskalMST.cpp \
       src/common/PrimMST.cpp \
       src/common/MSTFactory.cpp \
       src/common/MSTMetrics.cpp

OBJS = $(SRCS:.cpp=.o)

INCLUDES = -Isrc \
           -Isrc/server \
           -Isrc/utils \
           -Isrc/client \
           -Isrc/common

EXEC = server
CLIENT_EXEC = client

all: $(EXEC) $(CLIENT_EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

client_main.o: client_main.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/client/Client.o: src/client/Client.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(CLIENT_EXEC): client_main.o src/client/Client.o
	$(CXX) $(CXXFLAGS) -o $@ client_main.o src/client/Client.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC) $(CLIENT_EXEC) client_main.o src/client/Client.o memcheck_server.txt memcheck_client.txt helgrind_server.txt helgrind_client.txt

# Valgrind targets for server
memcheck_server: $(EXEC)
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=memcheck_server.txt ./$(EXEC)

helgrind_server: $(EXEC)
	valgrind --tool=helgrind --log-file=helgrind_server.txt ./$(EXEC)

# Valgrind targets for client
memcheck_client: $(CLIENT_EXEC)
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=memcheck_client.txt ./$(CLIENT_EXEC)

helgrind_client: $(CLIENT_EXEC)
	valgrind --tool=helgrind --log-file=helgrind_client.txt ./$(CLIENT_EXEC)

# Run all Valgrind checks
valgrind_check: memcheck_server helgrind_server memcheck_client helgrind_client

.PHONY: all clean memcheck_server helgrind_server memcheck_client helgrind_client valgrind_check