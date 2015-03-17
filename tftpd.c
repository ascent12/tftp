#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "network.h"

#define PACKET_SIZE 512

int wait_connection(int srv_sock, char *buffer, ssize_t *nread)
{
	int client_sock;
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof addr;

	memset(&addr, 0, sizeof addr);

	*nread = recvfrom(srv_sock, buffer, PACKET_SIZE, 0,
			(struct sockaddr *)&addr, &addr_len);

	if (*nread == -1)
		return -1;

	client_sock = socket(addr.ss_family, SOCK_DGRAM, 0);
	if (client_sock == -1) {
		fprintf(stderr, "Unable to create socket: %s\n", strerror(errno));
		return -1;
	}

	if (connect(client_sock, (struct sockaddr *)&addr, addr_len) == -1) {
		fprintf(stderr, "Unable to create socket connection: %s\n", strerror(errno));
		close(client_sock);
		return -1;
	}

	return client_sock;
}

int main(int argc, char *argv[])
{
	int srv_sock;

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return EXIT_FAILURE;
	}

	srv_sock = open_socket(NULL, argc == 2 ? argv[1] : "69",
			AF_INET6, AI_PASSIVE | AI_V4MAPPED, bind);

	while (1) {
		int client_sock;
		ssize_t nread = 0;
		char buffer[PACKET_SIZE];

		memset(buffer, 0, sizeof buffer);

		client_sock = wait_connection(srv_sock, buffer, &nread);

		printf("Recieved Packet of size %ld\n", nread);

		for (int i = 0; i < nread; ++i)
			putchar(buffer[i] == '\0' ? '_' : buffer[i]);
		putchar('\n');

		fflush(stdout);

		close(client_sock);
	}

	close(srv_sock);
}
