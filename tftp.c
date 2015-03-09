#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#define TRANS_GET 1
#define TRANS_PUT 2

int open_socket(const char *addr, const char *port)
{
	int sock;		/* Socket FD */
	int status;
	struct addrinfo hints;
	struct addrinfo *res;	/* getaddrinfo results */
	struct addrinfo *p;	/* Pointer to walk results linked list */

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((status = getaddrinfo(addr, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo failed for %s: %s\n", addr, gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	for (p = res; p != NULL; p = p->ai_next) {
		if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
			continue;

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Unable to connect to %s:%s : %s\n", addr, port, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (p == NULL || connect(sock, p->ai_addr, p->ai_addrlen) < 0) {
		fprintf(stderr, "Unable to connect to %s:%s : %s\n", addr, port, strerror(errno));
		close(sock);
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(res);

	return sock;
}

void usage(const char *name)
{
	fprintf(stderr, "Usage: %s server-address[:port] {get|put} filename [local-filename]\n", name);
	exit(EXIT_FAILURE);
}

void parse_args(int argc, char *argv[],
		const char **srv_addr,
		const char **srv_port,
		int *trans_type,
		const char **srv_filename,
		const char **local_filename)
{
	if (argc < 4 || argc > 5) {
		fprintf(stderr, "Invalid number of options\n");
		usage(argv[0]);
	}

	*srv_addr = argv[1];
	*srv_port = NULL;

	for (char *ptr = argv[1]; *ptr != '\0'; ++ptr) {
		if (*ptr == ':') {
			*ptr = '\0';
			*srv_port = ptr + 1;

			break;
		}
	}

	if (*srv_port == NULL)
		*srv_port = "69";

	if (!(strcmp(argv[2], "get") == 0 && (*trans_type = TRANS_GET)) &&
	    !(strcmp(argv[2], "put") == 0 && (*trans_type = TRANS_PUT))) {
		fprintf(stderr, "Unrecognised option '%s'\n", argv[2]);
		usage(argv[0]);
	}

	*srv_filename = argv[3];
	*local_filename = argc == 5 ? argv[4] : argv[3];
}

int main(int argc, char *argv[])
{
	const char *srv_addr;
	const char *srv_port;
	int trans_type;
	const char *srv_filename;
	const char *local_filename;

	int sock;

	parse_args(argc, argv,
		   &srv_addr, &srv_port,
		   &trans_type,
		   &srv_filename, &local_filename);
	printf("%s %s %d %s %s\n", srv_addr, srv_port, trans_type, srv_filename, local_filename);

	sock = open_socket(srv_addr, srv_port);

	const char *message = "Hello, World!\n";
	send(sock, message, strlen(message), 0);

	close(sock);

	return EXIT_SUCCESS;
}
