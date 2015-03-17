#ifndef NETWORK_H
#define NETWORK_H

#include <sys/time.h>

static const struct timeval timeout = { .tv_sec = 5, .tv_usec = 0};
static const struct timeval no_timeout = { .tv_sec = 0, .tv_usec = 0};

int open_socket(const char *addr,
		const char *port,
		int family, int flags,
		int (*fn)(int sockfd, const struct sockaddr *addr,
			socklen_t addrlen));

#endif
