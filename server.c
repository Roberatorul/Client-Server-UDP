#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/uio.h>
#include <unistd.h>

/* Random unused port */
#define PORT 50000

#define MAX_BUFFER_LEN 1024

int main() {
	struct sockaddr_in servadr, cliaddr;
	int sockfd;
	char buffer[MAX_BUFFER_LEN];
	size_t buffer_size;

	/* Create a socket */
	if ((sockfd = socked(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	/* Clear garbage initialization data */
	memset(&servadr, 0, sizeof(servadr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	/* Fill server information*/
	servadr.sin_family = PF_INET; /* IPv4 */
	servadr.sin_port = htons(PORT);
	servadr.sin_addr.s_addr = INADDR_ANY; /* Receive from any ip */

	/* Bind socket with server address*/
	if (bind(sockfd, (const struct sockaddr *)&servadr, sizeof(servadr)) < 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	/* Receive message from any ip */
	socklen_t recv_message_len;
	ssize_t n = recvfrom(sockfd, buffer, MAX_BUFFER_LEN, 0,
						(const struct sockaddr *)&cliaddr, &recv_message_len);
	
	if (n < 0) {
		perror("recv failed");
		exit(EXIT_FAILURE);
	}

	buffer[n] = '\0';
	printf("[Client]: %s", buffer);

	/* Send message to client */
	strcpy(buffer, "Hello from server!\n");
	buffer_size = strlen(buffer);
	if (sendto(sockfd, buffer, buffer_size, 0, 
			  (const struct sockaddr *)&cliaddr, sizeof(cliaddr))) {
		perror("send failed");
		exit(EXIT_FAILURE);
	}
	printf("Message sent.\n");

	/* Close socket fd */
	close(sockfd);

	return 0;
}
