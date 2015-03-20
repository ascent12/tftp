/*
 * Scott Anderson
 * #1240847
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "network.h"
#include "protocol.h"

void send_read(int sock, const char *filename)
{
	char buffer[PKT_SIZE] = {0,1};
	size_t len = 2;

	len += sprintf(buffer + len, "%.503s", filename);
	buffer[len++] = '\0';

	len += sprintf(buffer + len, "octet");
	buffer[len++] = '\0';

	if (send(sock, buffer, len, 0) == -1)
		perror("send_read");
}

void send_ack(int sock, uint16_t block_id)
{
	char buffer[4] = {0,4};

	block_id = htons(block_id);
	memcpy(buffer + 2, &block_id, sizeof block_id);

	if (send(sock, buffer, sizeof buffer, 0) == -1)
		perror("send_ack");
}

void send_error(int sock, uint16_t error_code, const char *error_msg)
{
	char buffer[PKT_SIZE] = {0,5};
	size_t len = 4;

	error_code = htons(error_code);
	memcpy(buffer + 2, &error_code, sizeof error_code);

	len += sprintf(buffer + len, "%.507s", error_msg);
	buffer[len++] = '\0';

	if (send(sock, buffer, len, 0) == -1)
		perror("send_error");
}

void send_data(int sock, uint16_t block_id, const char *data, size_t data_len)
{
	char buffer[PKT_SIZE] = {0,3};

	block_id = htons(block_id);
	memcpy(buffer + 2, &block_id, sizeof block_id);

	assert(data_len <= MAX_DATA_SIZE);
	memcpy(buffer + 4, data, data_len);

	if (send(sock, buffer, data_len + 4, 0) == -1)
		perror("send_data");
}

int pkt_op(const char *buffer)
{
	uint16_t opcode;

	memcpy(&opcode, buffer, sizeof opcode);

	switch (ntohs(opcode)) {
	case 1:
		return PKT_RRQ;
	case 2:
		return PKT_WRQ;
	case 3:
		return PKT_DATA;
	case 4:
		return PKT_ACK;
	case 5:
		return PKT_ERROR;
	default:
		return PKT_NONE;
	}
}

uint16_t pkt_blk_id(const char *buffer)
{
	uint16_t block_id;
	
	memcpy(&block_id, buffer + 2, sizeof block_id);

	return ntohs(block_id);
}

const char *pkt_data(const char *buffer)
{
	return buffer + 4;
}

const char *pkt_filename(const char *buffer)
{
	return buffer + 2;
}

const char *pkt_mode(const char *buffer)
{
	/* Skip the opcode */
	buffer += 2;

	while (*buffer++ != '\0');

	return buffer;
}
