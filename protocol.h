#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>

#define PACKET_SIZE 512

bool send_read(int sock, const char *filename);

#endif
