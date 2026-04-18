# compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -g -std=c++17 -I../common

# executables
TARGETS = server client

all: $(TARGETS)

# server
server: server.cpp ../common/UdpSocket.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# client
client: client.cpp ../common/UdpSocket.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

# create a random file
test_file:
	dd if=/dev/urandom of=test.bin bs=1M count=5
	@echo "File test.bin (5MB) was successfully created!"

clean:
	rm -rf $(TARGETS) ./output test.bin

.PHONY: all clean test_file
