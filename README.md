# UDP Client-Server architecture in C++

## Description
This project aims to implement a **custom client-server architecture** over UDP in C++. The repository is divided into 3 main directories, each adding more complexity to the architecture.

### Short directories description
* `raw-udp`
	* contains a basic client-server implementation.
	* one client at a time can access the server (no multithreading)
	* no packet loss handling
* `Go-Back-N-udp`
	* built on top of **raw-udp**
	* implements ACK to UDP
	* handles packet loss, timeouts, and retransmissions.
	* one client at a time can access the server (no multithreading)
* `parallel-udp`
	* built on top of **Go-Back-N-udp**
	* multiple clients can connect to the server

> **Note:** For more information about implementation, check each directory **README.md**

## Project structure
```bash
├── common # Contains basic classes used across directories
├── raw-udp # Basic client-server implementation with no ACK
├── Go-Back-N-udp # Implementation with ACKs
├── parallel-udp # Multithreaded implementation
```

## Prerequisites
* **C++17** or higher
* **g++** compiler and **make** installed
* A Posix-compliant environment (Linux, macOS, or WSL on Windows) for the Socket API

## TODOs
* [x] Implement `raw-udp` base logic
* [x] Implement `Go-Back-N-udp` (Sliding Window & Timeouts)
* [] Implement `parallel-udp` (Multithreading)
