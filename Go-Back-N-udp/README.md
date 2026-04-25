# Go-Back-N-udp

## Description
This project is an extension of `raw_udp/`. The core idea is the same, implement a client which sends a file to a server. Unlike the first implementation, the server has a reliable implementation, which means that every file sent from the client will be received completely.

## Workflow
* server
	* creates an **UdpSocket** and binds it port 50001 (**bindToPort()** creates *struct sockaddr_in* and fills server fields accordingly before binding, see **../common/README.md**)
	* enters in an infinite loop (waits for the client to send a file)
	* receives a packet from the client and checks its seq_num. based on that it either writes the packet into the recv_file if the received seq_num is the same as the expected seq_num or doesn't
	* whichever the case is, at the end, in both cases it sends back to the receiver an **ACK** with the expected_seq (expected_seq or expected_seq - 1 is the last well-received packet)

* client
	* takes server_ip and file_path from the command line (check **How to run** section)
	* creates an UdpSocket and sets a timeout on it (check `Mentions` section below for more information)
	* fills server information(knowing the server's configuration, fields are hardcoded rather than using **getaddrinfo()**)
	* before the sending loop starts, we define some key variable: `first_unconfirmed_packet`, `next_seq` which is the next sequence to be sent to the server and `dq` which is a double-ended-queue for storing the packets that we want to send
	* a **deque** was used because it offers both **vector** contiguous memory alignment (not all elements from deque are stored one after another like in vector, but parts from it are) which is good for CPU Caching and **list** fast deletion of elements
	* enters in an infinite loop (until it reaches EOF) and starts sending packets to the server:  
		first of all it sends WINDOW_SIZE packets to the server (and also pushes those packets into dq) without waiting for an ACK. this loop was used for this
		```cpp
		while (next_seq < first_unconfirmed_packet + WINDOW_SIZE && !eof_reached) {
			/* Code */
		}
		```
		first_unconfirmed_packet and next_seq will be increased in this loop so that's why we use **next_seq < first_unconfirmed_packet + WINDOW_SIZE**
		for sending WINDOW_SIZE packets from deque
	* after this loop it waits for a response from the server and acts accordingly:
		* if the socket timeout was received, it resends all the packets from dq (WINDOW_SIZE packets, this is one of the reasons why we store sent packets into dq)
		* if it received an ACK, it checks:
		```cpp
		if (ack_pkt.seq_num >= first_unconfirmed_packet) {
			/*
				It means that we got a good packet. > is used because in this transmision, if the server sends for example an ACK for packet 7, but the client was expecting an ACK for 5, it means that the server tried before to send an ACK for 5 and 6 but they were lost. This informs the the client that 5, 6 and 7 were received by the server and it can move its window to the next packets
			*/
		}
		/* Delete confirmed_packets elements from dq */
		```
		* after this first_unconfirmed_packet moves to ack_pkt.seq_num + 1
		* if EOF was reached or dq is empty it stops the sending loop

> **Note:** For debugging or simply a better understanding of what's happening, uncomment **cout's** from both server and client (this will affect performances).

### Mentions
The purpose of this project is to implement the `Go-Back-N protocol` with ACK's and timers. This means that the focus was more on protocol logic rather than **How big should the cliend window be?** or **How to set client socket timeout?**. For this you can play with:
```cpp
#define WINDOW_SIZE 25
#define WAITING_TIME_MS 500

client_socket.setReceiveTimeout(WAITING_TIME_MS);
```
and measure performance (check `Performance measurements` below).

## How to run
First on the `server side` we need to run:
```bash
mkdir output/ # otherwise we won't have where to store received files
ifconfig # for server_ip (our device ip)
make server # or make
./server
```

Then on other computer (or other terminal) for `client`:
```bash
make client # or make
# To create a random 5MB file
# make test_file 
./client server_ip file_path # or simply 0.0.0.0 for server_ip
```

## Performance measurements
In `client.cpp` we have the following method for measuring performance:
```cpp
#include <chrono> // for performance measuring

int main() {
	/* Code */
	std::cout << "Start transfer...\n";
	/* Start clock */
	auto begin = std::chrono::high_resolution_clock::now(); // measuring performance
    while (true) {
		/* Code */
		std::cout << "[CLIENT] File " << file_path << " was sent!\n\n";

		/* Stop clock */
		auto end = std::chrono::high_resolution_clock::now();
		/* Compute elapsed time */ 
		std::chrono::duration<double> elapsed_seconds = end - begin;
		std::cout << "[PERFORMANCE] Total time: " << elapsed_seconds.count() << " seconds\n";
	}
}
```

For a better measuring in `client` use:
```bash
make test_file
./client server_ip test.bin # or 0.0.0.0 for server_ip
```
