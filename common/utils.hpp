#pragma once

#include <iostream>
#include <cstdint>
#include <cstring>

#define MAX_PAYLOAD_LEN 1024

enum class PacketType : uint8_t {
	FILENAME,
	DATA,
	EOF_PKT,
	ACK // Used for Go-Back-N
};

struct Packet {
	PacketType type;
	uint32_t seq_num; // Used for Go-Back-N
	uint32_t len; // Number of valid bytes in the payload
	char payload[MAX_PAYLOAD_LEN]; // Actual data buffer

	Packet() : type(PacketType::DATA), seq_num(0), len(0) {
		std::fill(std::begin(payload), std::end(payload), 0);
	}

	void fill_packet(PacketType t, uint32_t seq, uint32_t l, 
					const char* data);
};
