#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "../common/utils.hpp"
#include "../common/UdpSocket.hpp"

/* Random unused port */
#define PORT 50001

int main() {
	try {
		UdpSocket server_socket;

		server_socket.bindToPort(PORT);
		std::cout << "Server listens to port: " << PORT << "...\n";

		Packet pkt;
		struct sockaddr_in cli_addr;
		int output_file = -1;
		int rc;
		size_t num_recv_files = 1;
		uint32_t expected_seq = 0;

		while (1) {
			/* Clear gaarbage data */
			memset(&cli_addr, 0, sizeof(cli_addr));

			/* Receive message from any ip */
			ssize_t n = server_socket.recvPacket(pkt, cli_addr);
			if (n < 0)
				continue;

			//std::cout << "Received packet with " << pkt.seq_num << " seq_num\n";
			if (pkt.seq_num == expected_seq) {
				if (pkt.type == PacketType::FILENAME) {
					pkt.payload[pkt.len] = '\0';

					/* Build file path */
					std::string file_path = "./output/" + std::to_string(num_recv_files)
											+ "." + std::string(pkt.payload);
					std::cout << "[SERVER] Start receiving file: " << file_path << "\n\n";

					output_file = open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (output_file < 0)
						throw std::runtime_error("Reading from file " + file_path + " error");
				} else if (pkt.type == PacketType::EOF_PKT) {
					std::cout << "\nReceived EOF. Stop writing to file\n";
					if (output_file >= 0) {
						close(output_file);
						output_file = -1; // Reset file descriptor
						std::cout << "[SERVER] File received\n\n";
					}
					num_recv_files++;
				} else if (pkt.type == PacketType::DATA && output_file >= 0) {
					rc = write(output_file, pkt.payload, pkt.len);
					if (rc < 0)
						throw std::runtime_error("Write failed");
				}

				/* Send ACK */
				//std::cout << "Sending ACK for " << expected_seq << "...\n";
				Packet ack_pkt;
				ack_pkt.fill_packet(PacketType::ACK, expected_seq, 0, nullptr);
				server_socket.sendPacket(ack_pkt, cli_addr);
				//std::cout << "ACK sent\n\n";

				if (pkt.type == PacketType::EOF_PKT)
					expected_seq = 0;
				else
					expected_seq++;
			} else {
				if (expected_seq > 0) {
					Packet ack_pkt;
					ack_pkt.fill_packet(PacketType::ACK, expected_seq - 1, 0, nullptr);
					//std::cout << "Received wrong packet. Resending ACK for "
					//		  << expected_seq - 1 << '\n';
					server_socket.sendPacket(ack_pkt, cli_addr);
				}
			}
		}
	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
