#ifndef NETWORK_H
#define NETWORK_H

int open_socket(const char *addr,
		const char *port,
		int family, int flags,
		int (*fn)(int sockfd, const struct sockaddr *addr,
			socklen_t addrlen));

#endif
