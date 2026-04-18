#pragma once

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include "./utils.hpp"

class UdpSocket {
private:
	int sockfd;

public:
	UdpSocket();
	~UdpSocket();

	int getFd() const;

	void bindToPort(int port, const std::string& ip_address = "0.0.0.0");
	void sendPacket(const Packet& pkt, const struct sockaddr_in& dest_addr);
	ssize_t recvPacket(Packet& pkt, struct sockaddr_in& sender_addr);
};
