#include <stdio.h>     
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>     
#include <sys/types.h>     
#include <sys/socket.h>     
#include <arpa/inet.h>     
#include <netinet/in.h>

/*
	Leave it like this if you are testing on the same computer,
	otherwise change it with server ip address
*/
#define SERVER_ADDRESS INADDR_ANY

/* Random unused port */
#define PORT 50000

#define MAX_BUFFER_LEN 1024

int main() {
	struct sockaddr_in servadr;
	int sockfd;
	char buffer[] = "Hello from client!\n";
	size_t buffer_size;

	/* Create a socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servadr, 0, sizeof(servadr));

	/* Fill server information */
	servadr.sin_family = PF_INET;
	servadr.sin_port = htons(PORT);
	servadr.sin_addr.s_addr = htonl(SERVER_ADDRESS);

	/* Send message */
	buffer_size = strlen(buffer);
	if (sendto(sockfd, buffer, buffer_size, 0, 
			  (const struct sockaddr *)&servadr, sizeof(servadr)) < 0) {
		perror("send failed");
		exit(EXIT_FAILURE);
	}
	printf("Message sent.\n");

	/* Receive message from server */
	socklen_t recv_message_len = sizeof(servadr);
	ssize_t n = recvfrom(sockfd, buffer, MAX_BUFFER_LEN, 0,
						(struct sockaddr *)&servadr, &recv_message_len);

	if (n < 0) {
		perror("recv failed");
		exit(EXIT_FAILURE);
	}

	buffer[n] = '\0';
	printf("[SERVER]: %s", buffer);

	/* Close socket fd*/
	close(sockfd);

	return 0;
}
