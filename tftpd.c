#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "network.h"
#include "protocol.h"

#define PACKET_SIZE 512

bool validate_filename(const char *filename)
{
	if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
		return false;

	for (const char *ptr = filename; *ptr != '\0'; ++ptr)
		if (!isalpha(*ptr) && *ptr != '.')
			return false;

	return true;
}

int find_file(const char *filename)
{
	int fd = -1;
	DIR *tftp;
	struct dirent *d;

	tftp = opendir("./Tftp");
	if (tftp == NULL) {
		perror("opendir");
		return -1;
	}

	while ((errno = 0) || (d = readdir(tftp)) != NULL) {
		if (strcmp(d->d_name, filename) == 0) {
			fd = openat(dirfd(tftp), d->d_name, O_RDONLY);
			if (fd == -1)
				perror("openat");
			errno = 0;
			break;
		}
	}

	if (errno != 0) {
		perror("readdir");
	}

	closedir(tftp);

	return fd;
}

bool wait_for_connection(int sock, char *buffer, ssize_t *nread)
{
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof addr;

	memset(&addr, 0, sizeof addr);

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &no_timeout, sizeof no_timeout);

	*nread = recvfrom(sock, buffer, PACKET_SIZE, 0,
			(struct sockaddr *)&addr, &addr_len);
	if (*nread == -1) {
		perror("wait_for_connection recvfrom");
		return false;
	}

	if (connect(sock, (struct sockaddr *)&addr, addr_len) == -1) {
		perror("wait_for_connection connect");
		return false;
	}

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

	return true;
}

void disconnect(int sock)
{
	struct sockaddr_in6 addr;

	memset(&addr, 0, sizeof addr);
	addr.sin6_family = AF_UNSPEC;

	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) == -1)
		perror("disconnect");
}

int main(int argc, char *argv[])
{
	int sock;
	int fd;

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return EXIT_FAILURE;
	}

	fd = find_file("hello");

	sock = open_socket(NULL, argc == 2 ? argv[1] : "69",
			AF_INET6, AI_PASSIVE | AI_V4MAPPED, bind);

	while (1) {
		ssize_t nread = 0;
		char buffer[PACKET_SIZE];
		int fd;

		if (!wait_for_connection(sock, buffer, &nread))
			continue;

		switch (packet_op(buffer)) {
		case RRQ:
			if (strcmp(packet_mode(buffer), "octet") != 0) {
				send_error(sock, 4, "Illegal TFTP operation.");
				goto error;
			}

			if (!validate_filename(packet_filename(buffer))) {
				send_error(sock, 1, "File not found.");
				goto error;
			}

			fd = find_file(packet_filename(buffer));
			if (fd == -1) {
				send_error(sock, 1, "File not found.");
				goto error;
			}

			break;
		case WRQ:
			send_error(sock, 2, "Access violation.");
			goto error;
		default:
			goto error;
		}

		char data[MAX_DATA_SIZE];
		ssize_t fread;
		uint16_t block_seq = 1;

		while ((fread = read(fd, data, sizeof data)) > 0) {
retry:
			send_data(sock, block_seq, data, fread);

			nread = recv(sock, buffer, sizeof buffer, 0);
			if (nread == -1) {
				perror("nread");
				goto end;
			}

			switch (packet_op(buffer)) {
			case ACK:
				;
				uint16_t block_id = packet_block_id(buffer);
				if (block_id == block_seq) {
					++block_seq;
				} else if (block_id == block_seq - 1) {
					goto retry;
				} else {
					goto end;
				}

				break;
			default:
				break;
			}

			if (fread < MAX_DATA_SIZE)
				break;
		}

end:
		close(fd);

error:
		disconnect(sock);
	}

	close(fd);
	close(sock);
}
