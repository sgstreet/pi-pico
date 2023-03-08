/*
 * dyamixel.h
 *
 *  Created on: Jan 16, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _DYNAMIXEL_H_
#define _DYNAMIXEL_H_

#include <stdint.h>
#include <compiler.h>
#include <devices/half-duplex.h>

#define DYNAMIXEL_TIMEOUT_MSEC 10
#define DYNAMIXEL_MAX_PACKET_SIZE 1024
#define DYNAMIXEL_MAX_PACKET_ID 252

#define DYNAMIXEL_PING_INSTR 0x01
#define DYNAMIXEL_READ_INSTR 0x02
#define DYNAMIXEL_WRITE_INSTR 0x03

#define DYNAMIXEL_STATUS_INSTR 0x55

enum dynamixel_reset_cmd
{
	DYNAMIXEL_RESET_NO_ID = 0x01,
	DYNAMIXEL_RESET_NO_ID_BAUD = 0x02,
	DYNAMIXEL_RESET_ALL = 0xff,
};

enum dynamixel_clear_cmd
{
	DYNAMIXEL_CLEAR_PRESENT_POSITION = 0x01,
};

enum dynamixel_backup_cmd
{
	DYNAMIXEL_BACKUP_STORE = 0x01,
	DYNAMIXEL_BACKUP_RESTORE = 0x02,
};

struct dynamixel_header
{
	uint8_t preamble[4];
	uint8_t packet_id;
	uint16_t length;
} __packed;

struct dynamixel_packet
{
	struct dynamixel_header header;
	uint8_t params[];
} __packed;

struct dynamixel_instr_packet
{
	struct dynamixel_header header;
	uint8_t instr;
	uint8_t params[];
} __packed;

struct dynamixel_status_packet
{
	struct dynamixel_header header;
	uint8_t instr;
	uint8_t error;
	uint8_t params[];
} __packed;

struct dynamixel_instruction
{
	uint8_t instruction;
} __packed;

struct dynamixel_status
{
	uint8_t instruction;
	uint8_t error;
} __packed;

struct dynamixel_ping_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
} __packed;

struct dynamixel_ping_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
	uint16_t model;
	uint8_t version;
} __packed;

struct dynamixel_read_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
	uint16_t address;
	uint16_t length;
} __packed;

struct dynamixel_read_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
	uint8_t params[];
} __packed;

struct dynamixel_write_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
	uint16_t address;
	uint8_t params[];
} __packed;

struct dynamixel_write_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
} __packed;

struct dynamixel_info
{
	uint8_t id;
	uint8_t version;
	uint16_t model;
};

struct dynamixel_port
{
	struct half_duplex *hd;
	uint8_t buffer[1024];
};

//struct dynamixel_port *dynamixel_open();
//struct dynamixel_port *dynamixel_close();

#define dynamixel_sizeof(type) (sizeof(type) - sizeof(struct dynamixel_header))

int dynamixel_port_ini(struct dynamixel_port *port, unsigned int channel, unsigned int baud_rate, unsigned int rx_timeout);
void dynamixel_port_fini(struct dynamixel_port *port);

struct dynamixel_port *dynamixel_port_create(unsigned int channel, unsigned int baud_rate, unsigned int rx_timeout);
void dynamixel_port_destroy(struct dynamixel_port *port);

int dynamixel_rx_packet(struct dynamixel_port *port, struct dynamixel_packet *packet, unsigned int timeout);
int dynamixel_tx_packet(struct dynamixel_port *port, struct dynamixel_packet *packet);
int dynamixel_transceive_packet(struct dynamixel_port *port, struct dynamixel_instr_packet *tx_packet, struct dynamixel_status_packet *rx_packet, unsigned int timeout);

int dynamixel_ping(struct dynamixel_port *port, uint8_t id, struct dynamixel_info *info, size_t count);
int dynamixel_reset(struct dynamixel_port *port, uint8_t id, enum dynamixel_reset_cmd reset_cmd);
int dynamixel_reboot(struct dynamixel_port *port, uint8_t id);
int dynamixel_clear(struct dynamixel_port *port, uint8_t id, enum dynamixel_clear_cmd clear_cmd);
int dynamixel_backup(struct dynamixel_port *port, uint8_t id, enum dynamixel_backup_cmd backup_cmd);

int dynamixel_read(struct dynamixel_port *port, uint8_t id, uint16_t addr, void *buffer, size_t count);
int dynamixel_write(struct dynamixel_port *port, uint8_t id, uint16_t addr, const void *buffer, size_t count);

#endif
