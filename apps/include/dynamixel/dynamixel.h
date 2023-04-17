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

#define DYNAMIXEL_ERRORS (__ELASTERROR + 100)
#define EDRESULT (DYNAMIXEL_ERRORS + 1)
#define EDINSTR  (DYNAMIXEL_ERRORS + 2)
#define EDCRC (DYNAMIXEL_ERRORS + 3)
#define EDRANGE (DYNAMIXEL_ERRORS + 4)
#define EDLEN (DYNAMIXEL_ERRORS + 5)
#define EDLIMIT (DYNAMIXEL_ERRORS + 6)
#define EDACCESS (DYNAMIXEL_ERRORS + 7)
#define EDALERT (DYNAMIXEL_ERRORS + 8)

#define DYNAMIXEL_TIMEOUT_MSEC 10
#define DYNAMIXEL_MAX_PACKET_SIZE 1024
#define DYNAMIXEL_MAX_PACKET_ID 252

#define DYNAMIXEL_BROADCAST_ID 0xfe

#define DYNAMIXEL_PING_INSTR 0x01
#define DYNAMIXEL_READ_INSTR 0x02
#define DYNAMIXEL_WRITE_INSTR 0x03
#define DYNAMIXEL_REG_WRITE_INSTR 0x04
#define DYNAMIXEL_ACTION_INSTR 0x05
#define DYNAMIXEL_FACTORY_RESET_INSTR 0x06
#define DYNAMIXEL_REBOOT_INSTR 0x08
#define DYNAMIXEL_CLEAR_INSTR 0x10

#define DYNAMIXEL_BACKUP_INSTR 0x20

#define DYNAMIXEL_STATUS_INSTR 0x55

#define DYNAMIXEL_SYNC_READ_INSTR 0x82
#define DYNAMIXEL_SYNC_WRITE_INSTR 0x83
#define DYNAMIXEL_FAST_SYNC_READ 0x8a

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

struct dynamixel_reg_write_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
	uint16_t address;
	uint8_t params[];
} __packed;

struct dynamixel_reg_write_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
} __packed;

struct dynamixel_action_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
} __packed;

struct dynamixel_action_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
} __packed;

struct dynamixel_reset_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
	uint8_t reset_cmd;
} __packed;

struct dynamixel_reset_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
} __packed;

struct dynamixel_reboot_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
} __packed;

struct dynamixel_reboot_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
} __packed;

struct dynamixel_clear_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
	uint8_t data[5];
} __packed;

struct dynamixel_clear_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
} __packed;


struct dynamixel_sync_read_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
	uint16_t addr;
	uint16_t length;
	uint8_t ids[];
} __packed;

struct dynamixel_sync_read_response
{
	struct dynamixel_header header;
	struct dynamixel_status status;
	uint8_t data[];
} __packed;

struct dynamixel_fast_sync_read_response
{
	struct dynamixel_header header;
	uint8_t instr;
	uint8_t data[];
} __packed;

struct dynamixel_sync_write_request
{
	struct dynamixel_header header;
	struct dynamixel_instruction instr;
	uint16_t addr;
	uint16_t length;
	uint8_t data[];
} __packed;

struct dynamixel_sync_write_response
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
	unsigned int rx_timeout;
	uint8_t buffer[1024];
};

#define dynamixel_sizeof(type) (sizeof(type) - sizeof(struct dynamixel_header))

int dynamixel_port_ini(struct dynamixel_port *port, unsigned int channel, unsigned int baud_rate, unsigned int idle_timeout);
void dynamixel_port_fini(struct dynamixel_port *port);

struct dynamixel_port *dynamixel_port_create(unsigned int channel, unsigned int baud_rate, unsigned int idle_timeout);
void dynamixel_port_destroy(struct dynamixel_port *port);

int dynamixel_port_rx_packet(struct dynamixel_port *port, struct dynamixel_packet *packet, unsigned int timeout);
int dynamixel_port_tx_packet(struct dynamixel_port *port, struct dynamixel_packet *packet);
int dynamixel_port_transceive_packet(struct dynamixel_port *port, struct dynamixel_instr_packet *tx_packet, struct dynamixel_status_packet *rx_packet, unsigned int timeout);

int dynamixel_port_ping(struct dynamixel_port *port, uint8_t id, struct dynamixel_info *info);
int dynamixel_port_reset(struct dynamixel_port *port, uint8_t id, enum dynamixel_reset_cmd reset_cmd);
int dynamixel_port_reboot(struct dynamixel_port *port, uint8_t id);
int dynamixel_port_clear(struct dynamixel_port *port, uint8_t id, enum dynamixel_clear_cmd clear_cmd);
int dynamixel_port_backup(struct dynamixel_port *port, uint8_t id, enum dynamixel_backup_cmd backup_cmd);

int dynamixel_port_read(struct dynamixel_port *port, uint8_t id, uint16_t addr, void *buffer, size_t count);
int dynamixel_port_write(struct dynamixel_port *port, uint8_t id, uint16_t addr, const void *buffer, size_t count);
int dynamixel_port_reg_write(struct dynamixel_port *port, uint8_t id, uint16_t addr, const void *buffer, size_t count);
int dynamixel_port_action(struct dynamixel_port *port, uint8_t id);
int dynamixel_port_sync_read(struct dynamixel_port *port, uint8_t *ids, size_t num_ids, uint16_t addr, uint16_t length, void *buffer);
int dynamixel_port_sync_write(struct dynamixel_port *port, uint8_t *ids, size_t num_ids, uint16_t addr, uint16_t length, const void *buffer);

size_t dynamixel_port_get_num_ports(void);
struct dynamixel_port *dynamixel_port_get_instance(unsigned int id);

#endif
