/*
 * Scott Anderson
 * #1240847
 */

/*
 * Scott Anderson
 * #1240847
 */

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

static int wait_for_data(int sock, char *buffer, ssize_t * nread)
{
	*nread = recv(sock, buffer, PKT_SIZE, 0);
	if (*nread == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			int errsav = errno;
			perror("recv");
			return (errno = errsav);
		}

		return (errno = EAGAIN);
	}

	if (pkt_op(buffer) == PKT_ERROR) {
		fprintf(stderr, "error: %s\n", pkt_err_msg(buffer));
		return (errno = ECONNABORTED);
	}

	if (pkt_op(buffer) != PKT_DATA)
		return (errno = EAGAIN);

	return (errno = 0);
}

static int download_file(int sock, int fd)
{
	char buffer[PKT_SIZE];
	ssize_t nread;
	uint16_t block_seq = 1;
	int i = 0;

	while (i++ < MAX_ATTEMPTS) {
		wait_for_data(sock, buffer, &nread);

		if (errno == EAGAIN)
			continue;
		else if (errno != 0)
			break;

		i = 0;

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

	if (errno == EAGAIN)
		fprintf(stderr, "recv: Timed out\n");

	if (errno != 0)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

static int request_file(const char *srv_addr, const char *srv_port,
			const char *file)
{
	int sock;
	struct sockaddr_storage addr = { 0 };
	socklen_t addr_len = sizeof addr;
	char buffer[PKT_SIZE];

	/* Connect to server's hosting port */
	sock = open_socket(srv_addr, srv_port, AF_UNSPEC, 0, connect);
	if (sock == -1)
		return -1;

	send_read(sock, file);

	if (getsockname(sock, (struct sockaddr *)&addr, &addr_len) == -1) {
		perror("getsockname");
		goto error;
	}

	disconnect(sock);

	/* Listen on local port for server's reply */
	if (bind(sock, (struct sockaddr *)&addr, addr_len) == -1) {
		perror("bind");
		goto error;
	};

	if (!set_timeout(sock))
		fprintf(stderr, "Warning: Socket may block indefinitely\n");

	memset(&addr, 0, sizeof addr);
	addr_len = sizeof addr;

	if (recvfrom(sock, buffer, sizeof buffer, MSG_PEEK,
		     (struct sockaddr *)&addr, &addr_len) == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			fprintf(stderr, "recvfrom: Server did not reply\n");
		else
			perror("recvfrom");

		goto error;
	}

	if (pkt_op(buffer) == PKT_ERROR) {
		fprintf(stderr, "error: %s\n", pkt_err_msg(buffer));
		goto error;
	}

	/* Connect with server's data transfer port */
	if (connect(sock, (struct sockaddr *)&addr, addr_len) == -1) {
		perror("connect");
		goto error;
	}

	return sock;

 error:
	close(sock);
	return -1;
}

static void parse_args(int argc, char *argv[],
		       const char **srv_addr,
		       const char **srv_port,
		       const char **srv_file, const char **local_file)
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

	sock = request_file(srv_addr, srv_port, srv_file);
	if (sock == -1)
		return EXIT_FAILURE;

	fd = creat(local_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		perror("creat");
		goto error_file;
	}

	ret = download_file(sock, fd);

	close(fd);

 error_file:
	close(sock);

	return ret;
}
