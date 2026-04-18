#include <stdio.h>     
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>     
#include <sys/types.h>     
#include <sys/socket.h>     
#include <arpa/inet.h>     
#include <netinet/in.h>
#include <fcntl.h>
#include "../common/utils.h"

/* Random unused port */
#define PORT 50000

void send_file(int sockfd, struct sockaddr_in servaddr, const char* file_name) {
	int input_file = open(file_name, O_RDONLY);
	DIE(input_file < 0, "open");
	
	Packet pkt;
	int rc;

	/* Send first message with received_file_name */
	// Fill packet type
	pkt.type = PKT_FILENAME;
	
	// Get rid of absolute path markers if there are any
	const char* base_name = strrchr(file_name, '/');
	base_name = base_name ? base_name + 1 : file_name;

	snprintf(pkt.payload, MAX_PAYLOAD_LEN, "recv_%s", base_name);
	pkt.len = strlen(pkt.payload);

	printf("Sending received file name...\n");
	if (sendto(sockfd, &pkt, sizeof(pkt), 0, 
			  (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("send failed");
		exit(EXIT_FAILURE);
	}
	printf("File name sent.\n");

	size_t packet_number = 1;
	while (1) {
		/* Reset pkt fields to 0 */
		memset(&pkt, 0, sizeof(pkt));

		/* Read data from input_file */
		rc = read(input_file, pkt.payload, MAX_PAYLOAD_LEN);
		DIE(rc < 0, "read");

		/* Reached EOF */
		if (rc == 0)
			break;

		/* Fill pkt fields */
		pkt.len = rc;
		pkt.type = PKT_DATA;

		/* Send packet */
		printf("Sending packet %zu...\n", packet_number);
		if (sendto(sockfd, &pkt, sizeof(pkt), 0, 
				  (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
			perror("send failed");
			exit(EXIT_FAILURE);
		}
		printf("Packet %zu sent.\n", packet_number);

		/* Increment the number of sent packets */
		packet_number++;
	}

	/* Sent EOF packet */
	memset(&pkt, 0, sizeof(pkt));
	pkt.type = PKT_EOF;
	pkt.len = 0;

	printf("Sending EOF...\n");
	if (sendto(sockfd, &pkt, sizeof(pkt), 0, 
			  (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("send failed");
		exit(EXIT_FAILURE);
	}
	printf("EOF sent.\n");

	printf("File %s was sent!\n\n", file_name);
}

int main(int argc, char* argv[]) {
	struct sockaddr_in servaddr;
	int sockfd;

	if (argc != 3) {
        fprintf(stderr, "Use: %s <server_IP_address> <path_to_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	/* Create a socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	/* Fill server information */
	servaddr.sin_family = PF_INET;
	servaddr.sin_port = htons(PORT);

	/* Fill server ip from command line argument */
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

	/* Send message */
	send_file(sockfd, servaddr, argv[2]);

	/* Close socket fd*/
	close(sockfd);

	return 0;
}
