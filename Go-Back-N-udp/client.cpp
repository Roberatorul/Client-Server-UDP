#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <deque>
#include <chrono>

#include "../common/UdpSocket.hpp"
#include "../common/utils.hpp"

/* Random unused port */
#define PORT 50000

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

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Use " << argv[0] << " <server_IP_address> <path_to_file>\n";
        return 1;
    }

	std::string server_ip = argv[1];
    std::string file_path = argv[2];
	int rc;

	try {
		UdpSocket client_socket;

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
		char buffer[MAX_PAYLOAD_LEN];
		
        uint32_t first_unconfirmed_packet = 0;
		uint32_t next_seq = 0; /* Next seq to be sent */
		bool eof_reached = false;

		std::cout << "Start transfer...\n";
		auto begin = std::chrono::high_resolution_clock::now(); // measuring performance
		while (true) {
			while (next_seq < first_unconfirmed_packet + WINDOW_SIZE && !eof_reached) {
				Packet send_pkt;

				if (next_seq == 0) { // First packet is the filename for server
					//std::cout << "Filename packet is built...\n";
					/* Build packet payload */
					// Get rid of absolute path markers if there are any
					size_t pos = file_path.find_last_of('/');
					std::string base_name = (pos == std::string::npos) ? file_path : file_path.substr(pos + 1);
					std::string dest_name = "recv_" + base_name;
					

					//std::cout << "Building FILENAME packet...\n";
					send_pkt.fill_packet(PacketType::FILENAME, next_seq, dest_name.length(), dest_name.c_str());
					//std::cout << "FILENAME packet built: " << dest_name << '\n' ;

				} else {
					//std::cout << "Reading data from file...\n";
					rc = read(input_file, buffer, MAX_PAYLOAD_LEN);

					if (rc < 0) {
						throw std::runtime_error("Reading from file error");
					} else if (rc == 0) {
						std::cout << "EOF reached\n\n";
						send_pkt.fill_packet(PacketType::EOF_PKT, next_seq, 0, nullptr);
						eof_reached = true;
					} else {
						send_pkt.fill_packet(PacketType::DATA, next_seq, rc, buffer);
					}
				}

					dq.push_back(send_pkt);
					//std::cout << "Trying to send packet...\n";
					client_socket.sendPacket(send_pkt, serv_addr);
					//std::cout << "Packet with " << next_seq << " seq_num was sent\n";
					next_seq++;
			}

			Packet ack_pkt;
			rc = client_socket.recvPacket(ack_pkt, serv_addr);

			/* Handle response from server */
            if (rc < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
					//std::cout << "[TIMEOUT] Retrying to send "
					//		  << first_unconfirmed_packet << '\n';

					/* Send from dq packets again */
                    for (const auto& p : dq) {
                        client_socket.sendPacket(p, serv_addr);
                        //std::cout << "[RETRANSMISION] Packet seq_num: " << p.seq_num << '\n';
                    }
                } else {
					throw std::runtime_error("Recvfrom error");
                }
            } else {
                if (ack_pkt.type == PacketType::ACK) {
                    //std::cout << "[RECEIVED ACK] ACK for seq_num: " << ack_pkt.seq_num << '\n';
                    
					/* Check if we got a good seq_num*/
                    if (ack_pkt.seq_num >= first_unconfirmed_packet) {
						/* Remove confirmed packets from dq */
                        uint32_t confirmed_packets = (ack_pkt.seq_num -
													 first_unconfirmed_packet) + 1;
                        for (uint32_t i = 0; i < confirmed_packets; ++i) {
                            if (!dq.empty()) {
                                dq.pop_front();
                            }
                        }
                        
						/* Move first_unconfirmed_packet */
                        first_unconfirmed_packet = ack_pkt.seq_num + 1;

                        if (eof_reached && dq.empty()) {
                            std::cout << "[CLIENT] File " << file_path << " was sent!\n\n";
                            break;
                        }
                    }
				} else {
					/* This should not be reached*/
					//std::cout << "Ignored packet! Client was waiting for PacketType::ACK\n";
				}
			}
		}

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - begin;
		std::cout << "[PERFORMANCE] Total time: " << elapsed_seconds.count() << " seconds\n";

        close(input_file);
	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << "\n";
	}

	return 0;
}
