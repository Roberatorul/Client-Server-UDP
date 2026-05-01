#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <deque>
#include <chrono>

#include "../common/UdpSocket.hpp"
#include "../common/utils.hpp"

/* Random unused port */
#define PORT 50001

#define FIRST_PACKET 0

/* Can vary */
#define WINDOW_SIZE 25
#define WAITING_TIME_MS 500

void fill_ipv4_address(struct sockaddr_in* addr, std::string addr_ip) {
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(PORT);

	/* Fill server ip */
	if (inet_pton(AF_INET, addr_ip.c_str(), &addr->sin_addr) <= 0)
        throw std::runtime_error("Invalid Ip address" + addr_ip);
}

/*
	* @brief Creates a packet into `pkt` with header specific format
	*
	* @pkt - will be filled with header specific format
	* @param file_path - will be set to the file that the client wants to send
	* @param seq_num - will be set to the `pkt` specific sequence
*/
void create_header_packet(Packet& pkt, const std::string& file_path, uint32_t seq_num) {
	//std::cout << "Filename packet is built...\n";

	/* Get rid of absolute path markers if there are any */
	size_t pos = file_path.find_last_of('/');
	std::string base_name = (pos == std::string::npos) ? file_path : file_path.substr(pos + 1);
	std::string dest_name = "recv_" + base_name;

	//std::cout << "Building FILENAME packet...\n";
	pkt.fill_packet(PacketType::FILENAME, seq_num, dest_name.length(), dest_name.c_str());
	//std::cout << "FILENAME packet built: " << dest_name << '\n' ;
}

/*
	* @brief Creates a packet into `pkt` with data read from `input_file`
	*
	* @param pkt - will be filled with data specific format
	* @param input_file - will be set to the file fd that client reads data from
	* @param seq_num - will be set to the `pkt` specific sequence
	*
	* Returns: number of bytes read from input_file with `read()`
*/
int create_packet_from_file(Packet& pkt, int input_file, int seq_num) {
	//std::cout << "Reading data from file...\n";
	char buffer[MAX_PAYLOAD_LEN];
	int rc = read(input_file, buffer, MAX_PAYLOAD_LEN);

	if (rc < 0) {
		throw std::runtime_error("Reading from file error");
	} else if (rc == 0) {
		std::cout << "EOF reached\n\n";
		pkt.fill_packet(PacketType::EOF_PKT, seq_num, 0, nullptr);
	} else {
		pkt.fill_packet(PacketType::DATA, seq_num, rc, buffer);
	}

	return rc;
}

/*
	* @brief Checks if an error or time expired occured
	*
	* @param client_socket - will be set to client socket in order to resend the packets from dq
	* @param serv_addr - will be set to the server address in order to resend the packets from dq
	* @param dq - contains the packets to be resent to dest_addr 
*/
void handle_receive_error(UdpSocket& client_socket, struct sockaddr_in dest_addr,
						  const std::deque<Packet>& dq) {
	if (errno == EWOULDBLOCK || errno == EAGAIN) { // Check if the server timeout expired
		//std::cout << "[TIMEOUT] Retrying to send "
		//		  << first_unconfirmed_packet << '\n';

		/* Send from dq packets again */
            for (const auto& p : dq) {
                client_socket.sendPacket(p, dest_addr);
                //std::cout << "[RETRANSMISION] Packet seq_num: " << p.seq_num << '\n';
            }
        } else {
			throw std::runtime_error("Recvfrom error");
        }
}

/*
	* @brief Handles received packet from server
	*
	* @param ack_pkt - will be set to the received packet
	* @param first_unconfirmed_packet - will be set to the first_unconfirmed_packet and then
	* 		 it will be set in function to the next unconfirmed_packet
	* @param dq - will be set to the packets dq and then, in function, the first confirmed
	* 		 packets will be poped out from dq
*/
void handle_received_packet(const Packet& ack_pkt, uint32_t& first_unconfirmed_packet,
							std::deque<Packet>& dq) {
	if (ack_pkt.type != PacketType::ACK) {
		/* This should not be reached*/
		std::cout << "Ignored packet! Client was waiting for PacketType::ACK\n";
		return;
	}

	/* Received an ACK packet */
    //std::cout << "[RECEIVED ACK] ACK for seq_num: " << ack_pkt.seq_num << '\n';

	/* Check if we got a good seq_num*/
    if (ack_pkt.seq_num >= first_unconfirmed_packet) {
		/* Remove confirmed packets from dq */
        uint32_t confirmed_packets = (ack_pkt.seq_num -
									  first_unconfirmed_packet) + 1;

        for (uint32_t i = 0; i < confirmed_packets; ++i)
            if (!dq.empty())
                    dq.pop_front();

		/* Move first_unconfirmed_packet */
        first_unconfirmed_packet = ack_pkt.seq_num + 1;
    }
}

void receive_packet(UdpSocket& client_socket, struct sockaddr_in serv_addr, std::deque<Packet>& dq,
					uint32_t& first_unconfirmed_packet) {
	Packet ack_pkt;
	int rc = client_socket.recvPacket(ack_pkt, serv_addr);

	/* Handle response from server */
    if (rc < 0) {
        handle_receive_error(client_socket, serv_addr, dq);
    } else {
        handle_received_packet(ack_pkt, first_unconfirmed_packet, dq);
	}
}

/*
	* @brief Sends a file to the server
	*
	* @param server_ip - will be set to the server_ip
	* @param file_path - will be set to the file that the client wants to send
*/
void send_file(std::string server_ip, std::string file_path) {
	UdpSocket client_socket;
	int rc;

	/* Can vary */
	client_socket.setReceiveTimeout(WAITING_TIME_MS);

	/* Fill server information */
	struct sockaddr_in serv_addr;
    fill_ipv4_address(&serv_addr, server_ip);

	//std::cout << "Opening file: " << file_path << "...\n";
	int input_file = open(file_path.c_str(), O_RDONLY);
    if (input_file < 0)
		throw std::runtime_error("Reading from file error");

	//std::cout << "File opened\n\n";

	std::deque<Packet> dq;

    uint32_t first_unconfirmed_packet = 0;
	uint32_t next_seq = 0; /* Next seq to be sent */
	bool eof_reached = false;

	std::cout << "Start transfer...\n";
	auto begin = std::chrono::high_resolution_clock::now(); // measuring performance
	while (true) {
		/* Send WINDOW_SIZE packets */
		while (next_seq < first_unconfirmed_packet + WINDOW_SIZE && !eof_reached) {
			Packet send_pkt;

			if (next_seq == FIRST_PACKET) { // First packet is the filename for server
				create_header_packet(send_pkt, file_path, next_seq);
			} else { // Normal packet
				rc = create_packet_from_file(send_pkt, input_file, next_seq);
				if (rc == 0)
					eof_reached = true;
			}

			dq.push_back(send_pkt);
			//std::cout << "Trying to send packet...\n";
			client_socket.sendPacket(send_pkt, serv_addr);
			//std::cout << "Packet with " << next_seq << " seq_num was sent\n";
			next_seq++;
		}

		receive_packet(client_socket, serv_addr, dq, first_unconfirmed_packet);

		if (eof_reached && dq.empty()) {
			std::cout << "[CLIENT] File " << file_path << " was sent!\n\n";
            break;
		}
	}

		/* Compute performance */
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - begin;
	std::cout << "[PERFORMANCE] Total time: " << elapsed_seconds.count() << " seconds\n";

    close(input_file);
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Use " << argv[0] << " <server_IP_address> <path_to_file>\n";
        return 1;
    }

	std::string server_ip = argv[1];
    std::string file_path = argv[2];

	try {
		send_file(server_ip, file_path);
	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << "\n";
	}

	return 0;
}
