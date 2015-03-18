#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "network.h"

int open_socket(const char *addr,
		const char *port,
		int family, int flags,
		int (*fn)(int sockfd, const struct sockaddr *addr,
			socklen_t addrlen))
{
	int sock = -1;
	int status;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = flags;

	status = getaddrinfo(addr, port, &hints, &res);
	if (status != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	for (p = res; p != NULL; p = p->ai_next) {
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == -1)
			continue;

		if (fn(sock, p->ai_addr, p->ai_addrlen) != -1)
			break;

		close(sock);
	}

	if (p == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);

	return sock;
}
