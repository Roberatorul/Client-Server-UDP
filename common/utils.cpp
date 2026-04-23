#include "utils.hpp"

Packet::Packet() : type(PacketType::DATA), seq_num(0), len(0) {
	std::fill(std::begin(payload), std::end(payload), 0);
}

void Packet::fill_packet(PacketType t, uint32_t seq, uint32_t l, 
						const char* data) {
	this->type = t;
	this->seq_num = seq_num;
	this->len = len;

	std::fill(std::begin(this->payload), std::end(this->payload), 0);

	if (data != nullptr && l > 0)
		memcpy(this->payload, payload, sizeof(this->payload));
}