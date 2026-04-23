# Common - Networking Wrapper & Utilities

## Description
This directory serves as the shared library for the entire project. It contains the wrappers used across all versions of the UDP client-server architecture (`raw-udp`, `Go-Back-N-udp`, and `parallel-udp`).

By centralizing this logic, we maintain a clean codebase for both the client and the server, allowing them to focus strictly on their specific application workflows rather than low-level socket management.

## Components

### 1. `utils.hpp`
This library contains basic data types:  
**Packet Type**
```cpp
enum class PacketType : uint8_t {
	FILENAME,
	DATA,
	EOF_PKT,
	ACK
};
```

**Packet Structure**
```cpp
struct Packet {
    PacketType type;
    uint32_t seq_num; // Used for Go-Back-N
    uint32_t len; // Number of valid bytes in the payload
    char payload[1024];   // Actual data buffer
};
```

### 2. `UdpSocket.hpp`
This library wraps an **int sockfd** used as an UdpSocket into its own class due to the need for separation between client/server logic and socket management. Furthermore, this helps us with automatic memory management of the socket (when the socket goes out of scope the destructor is automatically called and closes the socket).

**Udp Socket Class**
```cpp
class UdpSocket {
private:
	int sockfd;

public:
	UdpSocket(); // Creates an IPv4 socket
	~UdpSocket(); // Closes the file descriptor

	int getFd() const; // Returns socket file descriptor

	/* Binds the socket to a specific port */
	void bindToPort(int port, const std::string& ip_address = "0.0.0.0");

	/* Sends the pkt packet to a destination dest_addr */
	void sendPacket(const Packet& pkt, const struct sockaddr_in& dest_addr);

	/* 
	* Receives a packet into pkt and also fills sender address sender_addr
	* Return value is the one from recvfrom(), letting user decide how to handle errors
	*/
	ssize_t recvPacket(Packet& pkt, struct sockaddr_in& sender_addr);

	/* Set a recvfrom() timeout for file descriptor */
	void setReceiveTimeout(int milliseconds);
};
```

> **Note:** For implementation specifics check **UdpSocket.cpp**
