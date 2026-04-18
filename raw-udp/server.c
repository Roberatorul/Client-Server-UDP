#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <fcntl.h>
#include "../common/utils.h"

/* Random unused port */
#define PORT 50000

void receive_messages(int sockfd) {
	Packet pkt;
	int output_file = -1; // Mark output_file as invalid

	/* Sender waits for messages */
	while (1) {
		int rc;
		struct sockaddr_in cliaddr;
		memset(&cliaddr, 0, sizeof(cliaddr));

		/* Receive message from any ip */
		socklen_t recv_message_len = sizeof(cliaddr);
		ssize_t n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,
						(struct sockaddr *)&cliaddr, &recv_message_len);

		if (n < 0) {
			perror("recv failed");
			continue;
		}

		if (pkt.type == PKT_FILENAME) {
			pkt.payload[pkt.len] = '\0';
			printf("[SERVER] Satrt receiving file: %s\n", pkt.payload);

			output_file = open(pkt.payload, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			DIE(output_file < 0, "open");
			continue;
		}

		if (pkt.type == PKT_EOF) {
			if (output_file >= 0) {
				close(output_file);
				output_file = -1; // Reset file descriptor
				printf("[SERVER] File received\n\n");
			}
			continue;
		}

		if (pkt.type == PKT_DATA && output_file >= 0) {
			rc = write(output_file, pkt.payload, pkt.len);
			DIE(rc < 0, "write");
		}
	}
}

int main() {
	struct sockaddr_in servadr;
	int sockfd;

	/* Create a socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	/* Clear garbage initialization data */
	memset(&servadr, 0, sizeof(servadr));

	/* Fill server information*/
	servadr.sin_family = PF_INET; /* IPv4 */
	servadr.sin_port = htons(PORT);
	servadr.sin_addr.s_addr = INADDR_ANY; /* Receive from any ip */

	/* Bind socket with server address*/
	if (bind(sockfd, (const struct sockaddr *)&servadr, sizeof(servadr)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	/* Listen to incoming messages */
	receive_messages(sockfd);

	/* Close socket fd */
	close(sockfd);

	return 0;
}
