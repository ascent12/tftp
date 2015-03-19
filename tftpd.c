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
		if (strcmp(d->d_name, filename) != 0)
			continue;
		
		fd = openat(dirfd(tftp), d->d_name, O_RDONLY);
		if (fd == -1)
			perror("openat");

		errno = 0;
		break;
	}

	if (errno != 0)
		perror("readdir");

	closedir(tftp);

	return fd;
}

static bool wait_for_connection(int sock, char *buffer)
{
	struct sockaddr_storage addr = {0};
	socklen_t addr_len = sizeof addr;

	if (!remove_timeout(sock))
		return false;

	memset(buffer, 0, PKT_SIZE);

	if (recvfrom(sock, buffer, PKT_SIZE, 0,
			(struct sockaddr *)&addr, &addr_len) == -1) {
		perror("recvfrom");
		return false;
	}

	if (connect(sock, (struct sockaddr *)&addr, addr_len) == -1) {
		perror("connect");
		return false;
	}

	if (!set_timeout(sock))
		fprintf(stderr, "Warning: Socket may block indefinitely\n");

	return true;
}

static void disconnect(int sock)
{
	struct sockaddr_in6 addr = {0};
	addr.sin6_family = AF_UNSPEC;

	if (connect(sock, (struct sockaddr *)&addr, sizeof addr) == -1)
		perror("disconnect");
}

static int wait_for_ack(int sock, char *buffer)
{
	if (recv(sock, buffer, PKT_SIZE, 0) == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			int errsav = errno;
			perror("recv");
			return (errno = errsav);
		}

		return (errno = EAGAIN);
	}

	if (pkt_op(buffer) != PKT_ACK) {
		send_error(sock, 4, "Illegal TFTP operation.");
		return (errno = EAGAIN);
	}

	return (errno = 0);
}

static void transfer_file(int sock, int fd, char *buffer)
{
	char data[MAX_DATA_SIZE];
	ssize_t fread;
	uint16_t block_seq = 1;

	while ((fread = read(fd, data, sizeof data)) >= 0) {
		int i;

retry:
		i = 0;
		do
			send_data(sock, block_seq, data, fread);
		while (wait_for_ack(sock, buffer) == EAGAIN && i++ < MAX_ATTEMPTS);

		if (errno == EAGAIN)
			fprintf(stderr, "recv: Timed out\n");

		/* Timeout reached or connection failed */
		if (errno != 0)
			return;

		/* They probably didn't recieve our last DATA */
		if (pkt_blk_id(buffer) == block_seq - 1) {
			goto retry;
		} else if (pkt_blk_id(buffer) != block_seq) {
			send_error(sock, 0, "Incorrect block ID.");
			return;
		}

		++block_seq;

		/* Finished reading file */
		if (fread < MAX_DATA_SIZE)
			return;
	}

	if (fread == -1)
		send_error(sock, 0, "Unknown I/O error.");
}

static void handle_connection(int sock, char *buffer)
{
	int fd;

	if (pkt_op(buffer) == PKT_WRQ) {
		send_error(sock, 2, "Access violation.");
		return;
	} else if (pkt_op(buffer) != PKT_RRQ) {
		return;
	}

	if (strcmp(pkt_mode(buffer), "octet") != 0) {
		send_error(sock, 4, "Illegal TFTP operation.");
		return;
	}

	if (!validate_filename(pkt_filename(buffer))) {
		send_error(sock, 1, "File not found.");
		return;
	}

	fd = find_file(pkt_filename(buffer));
	if (fd == -1) {
		send_error(sock, 1, "File not found.");
		return;
	}

	transfer_file(sock, fd, buffer);

	close(fd);
}

int main(int argc, char *argv[])
{
	int sock;

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return EXIT_FAILURE;
	}

	sock = open_socket(NULL, argc == 2 ? argv[1] : "69",
			AF_INET6, AI_PASSIVE | AI_V4MAPPED, bind);
	if (sock == -1)
		return EXIT_FAILURE;

	bool running = true;
	while (running) {
		char buffer[PKT_SIZE];

		if (!wait_for_connection(sock, buffer))
			continue;

		if (strncmp(buffer, "quit", 4) == 0)
			running = false;

		handle_connection(sock, buffer);

		disconnect(sock);
	}

	close(sock);
}
