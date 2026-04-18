#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_PAYLOAD_LEN 1024

#define DIE(assertion, call_description)                                       \
do {                                                                         \
    if (assertion) {                                                           \
		fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
		perror(call_description);                                                \
		exit(errno);                                                             \
    }                                                                          \
} while (0)

typedef enum {
	PKT_FILENAME,
	PKT_DATA,
	PKT_EOF,
} PacketType;

typedef struct {
	PacketType type;
	size_t len;
	char payload[MAX_PAYLOAD_LEN];
} Packet;

#endif
