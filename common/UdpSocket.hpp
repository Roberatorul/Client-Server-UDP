#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <time.h>
#include "./utils.hpp"

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
