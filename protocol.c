#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "network.h"
#include "protocol.h"

bool send_read(int sock, const char *filename)
{
	char buffer[PACKET_SIZE];
	size_t len = 0;
	ssize_t nsend;

	buffer[0] = '0';
	buffer[1] = '1';
	len += 2;

	len += sprintf(buffer + len, "%.503s", filename);
	buffer[len++] = '\0';

	len += sprintf(buffer + len, "octet");
	buffer[len++] = '\0';

	nsend = send(sock, buffer, len, 0);
	if (nsend == -1) {
		perror("send_read");
		return false;
	}

	return true;
}

bool send_ack(int sock, uint16_t block_id)
{
	char buffer[4];
	ssize_t nsend;

	block_id = htons(block_id);

	buffer[0] = '0';
	buffer[1] = '4';
	memcpy(buffer + 2, &block_id, sizeof block_id);

	nsend = send(sock, buffer, sizeof buffer, 0);
	if (nsend == -1) {
		perror("send_ack");
		return false;
	}

	return true;
}

bool send_error(int sock, uint16_t error_code, const char *error_msg)
{
	char buffer[PACKET_SIZE];
	size_t len = 0;
	ssize_t nsend;

	error_code = htons(error_code);

	buffer[0] = '0';
	buffer[1] = '5';
	memcpy(buffer + 2, &error_code, sizeof error_code);
	len += 4;

	len += sprintf(buffer + len, "%.507s", error_msg);
	buffer[len++] = '\0';

	nsend = send(sock, buffer, len, 0);
	if (nsend == -1) {
		perror("send_error");
		return false;
	}

	return true;
}

bool send_data(int sock, uint16_t block_id, const char *data, size_t data_len)
{
	char buffer[PACKET_SIZE];
	size_t len = 0;
	ssize_t nsend;

	assert(data_len <= MAX_DATA_SIZE);

	block_id = htons(block_id);

	buffer[0] = '0';
	buffer[1] = '3';
	memcpy(buffer + 2, &block_id, sizeof block_id);
	len += 4;

	memcpy(buffer + len, data, data_len);
	len += data_len;

	nsend = send(sock, buffer, len, 0);
	if (nsend == -1) {
		perror("send_data");
		return false;
	}

	return true;
}

enum packet_type packet_op(const char *buffer)
{
	if (strncmp(buffer, "01", 2) == 0)
		return RRQ;
	else if (strncmp(buffer, "02", 2) == 0)
		return WRQ;
	else if (strncmp(buffer, "03", 2) == 0)
		return DATA;
	else if (strncmp(buffer, "04", 2) == 0)
		return ACK;
	else if (strncmp(buffer, "05", 2) == 0)
		return ERROR;
	else
		return NONE;
}

uint16_t packet_block_id(const char *buffer)
{
	uint16_t block_id;
	
	memcpy(&block_id, buffer + 2, sizeof block_id);

	return ntohs(block_id);
}

const char *packet_data(const char *buffer)
{
	return buffer + 4;
}

const char *packet_filename(const char *buffer)
{
	return buffer + 2;
}

const char *packet_mode(const char *buffer)
{
	while (*buffer++ != '\0');

	return buffer;
}
