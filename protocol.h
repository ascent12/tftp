#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define PACKET_SIZE 512
#define MAX_DATA_SIZE (PACKET_SIZE - 4)

#define MAX_ATTEMPTS 3

enum packet_type {
	NONE,
	RRQ,
	WRQ,
	DATA,
	ACK,
	ERROR
};

bool send_read(int sock, const char *filename);
bool send_ack(int sock, uint16_t block_id);
bool send_error(int sock, uint16_t error_code, const char *error_msg);
bool send_data(int sock, uint16_t block_id, const char *data, size_t data_len);

enum packet_type packet_op(const char *buffer);
uint16_t packet_block_id(const char *buffer);
const char *packet_data(const char *buffer);
#define packet_err_code packet_block_id
#define packet_err_msg packet_data
const char *packet_filename(const char *buffer);
const char *packet_mode(const char *buffer);

#endif
