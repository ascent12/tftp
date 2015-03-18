#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <sys/time.h>

int open_socket(const char *addr,
		const char *port,
		int family, int flags,
		int (*fn)(int sockfd, const struct sockaddr *addr,
			socklen_t addrlen));

bool set_timeout(int sock);
bool remove_timeout(int sock);

#endif
