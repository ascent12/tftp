#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#define PKT_SIZE 512
#define MAX_DATA_SIZE (PKT_SIZE - 4)

#define MAX_ATTEMPTS 3

#define PKT_NONE 0
#define PKT_RRQ 1
#define PKT_WRQ 2
#define PKT_DATA 3
#define PKT_ACK 4
#define PKT_ERROR 5

bool send_read(int sock, const char *filename);
bool send_ack(int sock, uint16_t block_id);
bool send_error(int sock, uint16_t error_code, const char *error_msg);
bool send_data(int sock, uint16_t block_id, const char *data, size_t data_len);

int         pkt_op(const char *buffer);
uint16_t    pkt_blk_id(const char *buffer);
const char *pkt_data(const char *buffer);
const char *pkt_filename(const char *buffer);
const char *pkt_mode(const char *buffer);
#define     pkt_err_code pkt_blk_id
#define     pkt_err_msg pkt_data

#endif
