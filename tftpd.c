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
	int fd;
	DIR *tftp;
	struct dirent *d;

	tftp = opendir("./Tftp");
	if (tftp == NULL) {
		fprintf(stderr, "Unable to open tftp directory: %s\n",
				strerror(errno));
		return -1;
	}

	while ((errno = 0) || (d = readdir(tftp)) != NULL) {
		if (strcmp(d->d_name, filename) == 0) {
			fd = openat(dirfd(tftp), d->d_name, O_RDONLY);
			if (fd == -1)
				fprintf(stderr, "Could not open %s: %s\n",
						d->d_name, strerror(errno));

			errno = 0;
			break;
		}
	}

	if (errno != 0) {
		fprintf(stderr, "Unable to read tftp directory: %s\n",
				strerror(errno));
	}

	closedir(tftp);

	return fd;
}

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
	int fd;

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [port]\n", argv[0]);
		return EXIT_FAILURE;
	}

	fd = find_file("hello");

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

	close(fd);
	close(srv_sock);
}
