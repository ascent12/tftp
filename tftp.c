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

static int download_file(int sock, int fd)
{
	char buffer[PKT_SIZE];
	ssize_t nread;
	uint16_t block_seq = 1;

	while ((nread = recv(sock, buffer, sizeof buffer, 0)) > 0) {
		if (pkt_op(buffer) == PKT_ERROR) {
			fprintf(stderr, "error: %s\n", pkt_err_msg(buffer));
			return EXIT_FAILURE;
		/* Silently discard all other packets */
		} else if (pkt_op(buffer) != PKT_DATA) {
			continue;
		}

		/* They probably didn't recieve our last ACK */
		if (pkt_blk_id(buffer) == block_seq - 1) {
			send_ack(sock, block_seq - 1);
			continue;
		} else if (pkt_blk_id(buffer) != block_seq) {
			fprintf(stderr, "error: Incorrect block ID\n");
			return EXIT_FAILURE;
		}

		if (write(fd, pkt_data(buffer), nread - 4) == -1) {
			perror("write");
			return EXIT_FAILURE;
		}

		send_ack(sock, block_seq++);

		/* They have finised sending */
		if (nread < PKT_SIZE)
			break;
	}

	if (nread == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			fprintf(stderr, "recv: Timeout reached\n");
		else
			perror("recv");

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static void parse_args(int argc, char *argv[],
		const char **srv_addr,
		const char **srv_port,
		const char **srv_file,
		const char **local_file)
{
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "Invalid number of options\n"
				"Usage: %s server-address[:port] file [local-file]\n",
				argv[0]);
		exit(EXIT_FAILURE);;
	}

	*srv_addr = argv[1];
	*srv_port = NULL;

	for (size_t i = strlen(argv[1]) - 1; i > 0; --i) {
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
	int ret = EXIT_FAILURE;
	int sock;
	int fd;

	const char *srv_addr;
	const char *srv_port;
	const char *srv_file;
	const char *local_file;

	parse_args(argc, argv, &srv_addr, &srv_port, &srv_file, &local_file);

	sock = open_socket(srv_addr, srv_port, AF_UNSPEC, 0, connect);
	if (sock == -1)
		return EXIT_FAILURE;

	if (!set_timeout(sock))
		goto error_sock;

	fd = creat(local_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (fd == -1) {
		perror("creat");
		goto error_sock;
	}

	send_read(sock, srv_file);
	ret = download_file(sock, fd);

	close(fd);
error_sock:
	close(sock);

	return ret;
}
