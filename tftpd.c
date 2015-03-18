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

static bool validate_filename(const char *filename)
{
	if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
		return false;

	for (const char *ptr = filename; *ptr != '\0'; ++ptr)
		if (!isalpha(*ptr) && *ptr != '.')
			return false;

	return true;
}

static int find_file(const char *filename)
{
	int fd = -1;
	DIR *tftp;
	struct dirent *d = NULL;

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

static bool wait_for_connection(int sock, char *buffer, ssize_t *nread)
{
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof addr;

	memset(&addr, 0, sizeof addr);

	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &no_timeout, sizeof no_timeout);

	*nread = recvfrom(sock, buffer, PKT_SIZE, 0,
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

static void disconnect(int sock)
{
	struct sockaddr_in6 addr;

	memset(&addr, 0, sizeof addr);
	addr.sin6_family = AF_UNSPEC;

	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) == -1)
		perror("disconnect");
}

static void check_other_connections(int sock)
{
	char buffer;
	struct sockaddr_storage addr;
	socklen_t addr_len = sizeof addr;
	ssize_t nread;

	memset(&addr, 0, sizeof addr);

//	printf("check_other_connections entered. FD: %d\n", sock);

	if (sock == -1)
		return;

	/* Error checking isn't important, because it doesn't matter if it fails */
	if ((nread = recvfrom(sock, &buffer, sizeof buffer, MSG_DONTWAIT,
			(struct sockaddr *)&addr, &addr_len)) > 0) {
		const char error_msg[] = "05\x00\x05Unknown transfer ID.";

		sendto(sock, error_msg, sizeof error_msg, 0,
			(struct sockaddr *)&addr, addr_len);
	}

}

	

int main(int argc, char *argv[])
{
	int sock;
	int err_sock;

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return EXIT_FAILURE;
	}

	sock = open_socket(NULL, argc == 2 ? argv[1] : "69",
			AF_INET6, AI_PASSIVE | AI_V4MAPPED, bind);

	err_sock = dup(sock);
	if (err_sock == -1)
		/* This is a non-fatal error
		 * We won't reply to non-connected people
		 */
		perror("dup");

	while (1) {
		ssize_t nread = 0;
		char buffer[PKT_SIZE];
		int fd;

		if (!wait_for_connection(sock, buffer, &nread))
			continue;

		switch (pkt_op(buffer)) {
		case PKT_RRQ:
			if (strcmp(pkt_mode(buffer), "octet") != 0) {
				send_error(sock, 4, "Illegal TFTP operation.");
				goto error;
			}

			if (!validate_filename(pkt_filename(buffer))) {
				send_error(sock, 1, "File not found.");
				goto error;
			}

			fd = find_file(pkt_filename(buffer));
			if (fd == -1) {
				send_error(sock, 1, "File not found.");
				goto error;
			}

			break;
		case PKT_WRQ:
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

			check_other_connections(err_sock);

			switch (pkt_op(buffer)) {
			case PKT_ACK:
				;
				uint16_t block_id = pkt_blk_id(buffer);
				if (block_id == block_seq) {
					++block_seq;
				} else if (block_id == block_seq - 1) {
					goto retry;
				} else {
					goto end;
				}

				break;
			default:
				printf("Invalid packet type\n");
				continue;
			}

			if (fread < MAX_DATA_SIZE)
				break;
		}

end:
		close(fd);

error:
		disconnect(sock);
	}

	shutdown(sock, SHUT_RDWR);
}
