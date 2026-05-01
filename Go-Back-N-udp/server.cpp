#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "../common/utils.hpp"
#include "../common/UdpSocket.hpp"

/* Random unused port */
#define PORT 50001

#define INVALID_FD -1

/*
	* @brief Creates a packet into `pkt` with header specific format
	*
	* @param pkt - will be filled with header specific format
	* @param output_file - returns the file descriptor for the file that
	*		 the server writes data to
	* @param num_recv_files - will be set to the number of received files (used
	* 		 for opening the file with the specific header format)
*/
void handle_filename_packet(Packet& pkt, int& output_file, size_t num_recv_files) {
	pkt.payload[pkt.len] = '\0';

	/* Build file path */
	std::string file_path = "./output/" + std::to_string(num_recv_files)
							+ "." + std::string(pkt.payload);
	std::cout << "[SERVER] Start receiving file: " << file_path << "\n\n";

	output_file = open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (output_file < 0)
		throw std::runtime_error("Reading from file " + file_path + " error");
}

/*
	* @brief Handles EOF case
	*
	* @param output_file - will be set to the file descriptor that the server
	* 		 writes data to and then set, in function, to INVALID_FD
*/
void handle_eof_packet(int& output_file) {
	std::cout << "\nReceived EOF. Stop writing to file\n";
	if (output_file >= 0) {
		close(output_file);
		output_file = INVALID_FD; // Reset file descriptor
		std::cout << "[SERVER] File received\n\n";
	}
}

/*
	* @brief Handles normal packet 
	*
	* @param pkt - will be set to the packet that the server received
	* @param output_file - will be set to the file descriptor that the server
	* 		 writes data to
*/
void handle_normal_packet(const Packet& pkt, int output_file) {
	int rc = write(output_file, pkt.payload, pkt.len);
	if (rc < 0)
		throw std::runtime_error("Write failed");
}

/*
	* @brief Sends an ACK for the received packet
	*
	* @param server_socket - will be set to the server socket
	* @param cli_addr - will be set to the client address (used for sending
	* 		 the ACK)
	* @param recv_pkt - will be set to the packet that the server received
	* @param expected_seq - will be set to the sequence that the server expects
	* 		 and then will be modified to the next expected sequence
*/
void send_ack_packet(UdpSocket& server_socket, struct sockaddr_in cli_addr,
					 const Packet& recv_pkt, uint32_t& expected_seq) {
	//std::cout << "Sending ACK for " << expected_seq << "...\n";
	Packet ack_pkt;
	ack_pkt.fill_packet(PacketType::ACK, expected_seq, 0, nullptr);
	server_socket.sendPacket(ack_pkt, cli_addr);
	//std::cout << "ACK sent\n\n";

	if (recv_pkt.type == PacketType::EOF_PKT)
		expected_seq = 0;
	else
		expected_seq++;
}

int main() {
	try {
		UdpSocket server_socket;

		server_socket.bindToPort(PORT);
		std::cout << "Server listens to port: " << PORT << "...\n";

		Packet recv_pkt;
		struct sockaddr_in cli_addr;
		int output_file = INVALID_FD;
		size_t num_recv_files = 1;
		uint32_t expected_seq = 0;

		while (1) {
			/* Clear gaarbage data */
			memset(&cli_addr, 0, sizeof(cli_addr));

			/* Receive message from any ip */
			ssize_t n = server_socket.recvPacket(recv_pkt, cli_addr);
			if (n < 0)
				continue;

			//std::cout << "Received packet with " << pkt.seq_num << " seq_num\n";
			if (recv_pkt.seq_num == expected_seq) {
				if (recv_pkt.type == PacketType::FILENAME) {
					handle_filename_packet(recv_pkt, output_file, num_recv_files);
				} else if (recv_pkt.type == PacketType::EOF_PKT) {
					handle_eof_packet(output_file);
					num_recv_files++;
				} else if (recv_pkt.type == PacketType::DATA && output_file >= 0) {
					handle_normal_packet(recv_pkt, output_file);
				}

				send_ack_packet(server_socket, cli_addr, recv_pkt, expected_seq);
			} else { // bad seq_num
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
