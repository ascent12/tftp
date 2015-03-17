#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "network.h"
#include "protocol.h"

void usage(const char *name)
{
	fprintf(stderr,
		"Usage: %s server-address[:port] file [local-file]\n",
		name);
	exit(EXIT_FAILURE);
}

void parse_args(int argc, char *argv[],
		const char **srv_addr,
		const char **srv_port,
		const char **srv_file,
		const char **local_file)
{
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Invalid number of options\n");
		usage(argv[0]);
	}

	*srv_addr = argv[1];
	*srv_port = NULL;

	for (int i = strlen(argv[1]) - 1; i >= 0; --i) {
		if (argv[1][i] == ':') {
			argv[1][i] = '\0';
			*srv_port = &argv[1][i + 1];

			break;
		} else if (!isdigit(argv[1][i])) {
			break;
		}
	}

	/* Server port was not provided */
	if (*srv_port == NULL || **srv_port == '\0')
		*srv_port = "69";

	*srv_file = argv[2];
	*local_file = argc == 4 ? argv[3] : argv[2];
}

int main(int argc, char *argv[])
{
	int ret = EXIT_SUCCESS;
	int sock;
	int fd;

	const char *srv_addr;
	const char *srv_port;
	const char *srv_file;
	const char *local_file;

	char buffer[PACKET_SIZE];
	ssize_t nread;
	uint16_t block_seq;

	parse_args(argc, argv,
		   &srv_addr, &srv_port,
		   &srv_file, &local_file);

	sock = open_socket(srv_addr, srv_port, AF_UNSPEC, 0, connect);
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

	fd = creat(local_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd == -1) {
		perror("creat");
		ret = EXIT_FAILURE;
		goto error_creat;
	}

	send_read(sock, srv_file);
	block_seq = 1;

	while (1) {
		nread = recv(sock, buffer, sizeof buffer, 0);
		if (nread == -1) {
			perror("recv");
			return EXIT_FAILURE;
		}

		switch (packet_op(buffer)) {
		case DATA:
			;
			uint16_t block_id = packet_block_id(buffer);

			if (block_id == block_seq) {
				if (write(fd, packet_data(buffer), nread - 4) == -1) {
					perror("write");
					ret = EXIT_FAILURE;
					goto error_write;
				}

				send_ack(sock, block_seq++);

				if (nread < PACKET_SIZE)
					goto exit;
			} else if (block_id == block_seq - 1) {
				send_ack(sock, block_id);
			} else {
				ret = EXIT_FAILURE;
				goto error_write;
			}

			break;
		case ERROR:
			fprintf(stderr, "error: (%d) %s\n", packet_err_code(buffer),
					packet_err_msg(buffer));
			ret = EXIT_FAILURE;
			goto error_write;
		default:
			/* Ignore */
			break;
		}
	}
exit:


error_write:
	close(fd);
error_creat:
	close(sock);

	return ret;
}
