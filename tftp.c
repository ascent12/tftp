#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>

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
	int sock;

	const char *srv_addr;
	const char *srv_port;
	const char *srv_file;
	const char *local_file;

	parse_args(argc, argv,
		   &srv_addr, &srv_port,
		   &srv_file, &local_file);
	printf("%s %s %s %s\n", srv_addr, srv_port, srv_file,
	       local_file);

	sock = open_socket(srv_addr, srv_port, AF_UNSPEC, 0, connect);

	send_read(sock, srv_file);

	close(sock);

	return EXIT_SUCCESS;
}
