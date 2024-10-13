CXX = g++
CXXFLAGS = -std=c++14 -pthread -g -O0 -fprofile-arcs -ftest-coverage
LDFLAGS = -lgcov --coverage

SRCS = src/server/server.cpp \
       src/utils/threadpool.cpp \
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
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

src/client/client.o: src/client/client.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(CLIENT_EXEC): src/client/client.o
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXEC) $(CLIENT_EXEC) src/client/client.o
	find . -name "*.gcno" -type f -delete
	find . -name "*.gcda" -type f -delete
	find . -name "*.gcov" -type f -delete

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

# Code coverage
coverage:
	$(MAKE) all
	./run_tests.sh
	lcov --capture --directory . --output-file coverage.info
	lcov --remove coverage.info '/usr/*' --output-file coverage.info
	genhtml coverage.info --output-directory coverage_report
	@echo "Coverage report generated. Open coverage_report/index.html in your browser."

clean_coverage:
	find . -name "*.gcda" -type f -delete
	find . -name "*.gcno" -type f -delete

.PHONY: all clean memcheck_server helgrind_server memcheck_client helgrind_client valgrind_check coverage clean_coverage