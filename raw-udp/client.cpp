#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <chrono>

#include "../common/UdpSocket.hpp"
#include "../common/utils.hpp"

/* Random unused port */
#define PORT 50000

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

		/* Fill server information */
		struct sockaddr_in serv_addr;
        std::memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

		/* Fill server ip */
		if (inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr) <= 0)
			throw std::runtime_error("Invalid IP address" + server_ip);

		int input_file = open(file_path.c_str(), O_RDONLY);
        if (input_file < 0)
			throw std::runtime_error("Open failed");

        Packet pkt;

		/* Send first message with received_file_name */
		// Fill packet type
        pkt.type = PacketType::FILENAME;

		// Get rid of absolute path markers if there are any
        size_t pos = file_path.find_last_of('/');
        std::string base_name = (pos == std::string::npos) ? file_path : file_path.substr(pos + 1);
        std::string dest_name = "recv_" + base_name;

        // Fill payload field
        strncpy(pkt.payload, dest_name.c_str(), MAX_PAYLOAD_LEN - 1);
        pkt.payload[MAX_PAYLOAD_LEN - 1] = '\0'; // Asiguram null-termination
        pkt.len = dest_name.length();

        std::cout << "[CLIENT] Sending file name: " << dest_name << "...\n";
        client_socket.sendPacket(pkt, serv_addr);

        //size_t packet_number = 1;
		std::cout << "Start transfer...\n";
		auto begin = std::chrono::high_resolution_clock::now(); // measuring performance
        while (true) {
            /* Reset pkt fields to 0 */
            memset(pkt.payload, 0, MAX_PAYLOAD_LEN);
            
            rc = read(input_file, pkt.payload, MAX_PAYLOAD_LEN);
            if (rc < 0)
				throw std::runtime_error("Open failed");

			/* Reached EOF */
            if (rc == 0)
                break;

			/* Fill pkt fields */
            pkt.type = PacketType::DATA;
            pkt.len = rc;

			/* Send packet */
            client_socket.sendPacket(pkt, serv_addr);
           // std::cout << "Packet " << packet_number++ << " sent.\n"; 
        }

		/* Send EOF file */
        pkt.type = PacketType::EOF_PKT;
        pkt.len = 0;

		std::cout << "Sending EOF...\n";
        client_socket.sendPacket(pkt, serv_addr);
		std::cout << "EOF sent\n";

        std::cout << "[CLIENT] File " << file_path << " was sent!\n\n";

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = end - begin;
		std::cout << "[PERFORMANCE] Total time: " << elapsed_seconds.count() << " seconds\n";

        close(input_file);
	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << "\n";
	}

	return 0;
}
