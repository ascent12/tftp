#include <errno.h>
#include <stdbool.h>
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
	struct addrinfo hints = {0};
	struct addrinfo *res;
	struct addrinfo *p;

	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = flags;

	int status = getaddrinfo(addr, port, &hints, &res);
	if (status != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}

	for (p = res; p != NULL; p = p->ai_next) {
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == -1)
			continue;

		if (fn == NULL || fn(sock, p->ai_addr, p->ai_addrlen) != -1)
			break;

		close(sock);
	}

	if (p == NULL) {
		perror("open_socket");
		sock = -1;
	}

	freeaddrinfo(res);

	return sock;
}

static const struct timeval timeout = { .tv_sec = 5, .tv_usec = 0};
bool set_timeout(int sock)
{
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout)) {
		perror("setsockopt");
		return false;
	}

	return true;
}

static const struct timeval no_timeout = { .tv_sec = 0, .tv_usec = 0};
bool remove_timeout(int sock)
{
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &no_timeout, sizeof no_timeout)) {
		perror("setsockopt");
		return false;
	}

	return true;
}

void disconnect(int sock)
{
	struct sockaddr_in6 addr = {0};
	addr.sin6_family = AF_UNSPEC;

	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) == -1)
		perror("disconnect");
}
