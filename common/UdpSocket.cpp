#include "../common/UdpSocket.hpp"

UdpSocket::UdpSocket() {
	this->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (this->sockfd < 0)
		throw std::runtime_error("Socket creation failed");
}

UdpSocket::~UdpSocket() {
	close(this->sockfd);
}

int UdpSocket::getFd() const {
	return this->sockfd;
}

void UdpSocket::bindToPort(int port, const std::string& ip_address) {
	struct sockaddr_in addr;

	/* Clear garbage initialization data */
	std::memset(&addr, 0, sizeof(addr));

	/* Fill server information*/
	addr.sin_family = PF_INET; /* IPv4 */
	addr.sin_port = htons(port);

	/* Fill ip */
	if (ip_address == "0.0.0.0" || ip_address.empty()) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, ip_address.c_str(), &addr.sin_addr) <= 0) {
            throw std::runtime_error("IP invalid pentru bind: " + ip_address);
        }
    }

	/* Bind socket with server address*/
	if (bind(this->sockfd, (const struct sockaddr *)&addr,
			sizeof(addr)) < 0)
		throw std::runtime_error("Bind to port" + std::to_string(port)
								+ "failed");

}

void UdpSocket::sendPacket(const Packet& pkt,
						  const struct sockaddr_in& dest_addr) {
	if (sendto(this->sockfd, &pkt, sizeof(Packet), 0, 
			  (const struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		throw std::runtime_error("Sendto failed");
	}
}

ssize_t UdpSocket::recvPacket(Packet& pkt, struct sockaddr_in& sender_addr) {
	socklen_t len = sizeof(sender_addr);
    ssize_t n = recvfrom(sockfd, &pkt, sizeof(Packet), 0, 
                        (struct sockaddr *)&sender_addr, &len);

    return n;
}

void UdpSocket::setReceiveTimeout(int milliseconds) {
	struct timeval tv;
	tv.tv_sec = milliseconds / 1000;
	tv.tv_usec = (milliseconds % 1000) * 1000;
	int rc = setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	if (rc < 0)
		throw std::runtime_error("Setsockopt failed");
}
