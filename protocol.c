#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>

#include "network.h"
#include "protocol.h"

bool send_read(int sock, const char *filename)
{
	char buffer[PACKET_SIZE];
	size_t len = 0;
	ssize_t nsend;

	buffer[0] = '0';
	buffer[1] = '1';
	len += 2;

	len += sprintf(buffer + len, "%.503s", filename);
	buffer[len++] = '\0';

	len += sprintf(buffer + len, "octet");
	buffer[len++] = '\0';

	printf("%zu\n", len);
	nsend = send(sock, buffer, len, 0);
	printf("%zd\n", nsend);
	if (nsend < 0) {
		fprintf(stderr, "Unable to send read packet: %s",
				strerror(errno));
		return false;
	}

	return true;
}
