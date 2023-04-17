/*
 * dynamixel.c
 *
 *  Created on: Jan 16, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/syslog.h>

#include <devices/devices.h>

#include <dynamixel/dynamixel.h>

static const uint16_t crc_table[256] =
{
	0x0000,	0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
	0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,	0x0022,
	0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D,	0x8077, 0x0072,
	0x0050, 0x8055, 0x805F, 0x005A, 0x804B,	0x004E, 0x0044, 0x8041,
	0x80C3, 0x00C6, 0x00CC, 0x80C9,	0x00D8, 0x80DD, 0x80D7, 0x00D2,
	0x00F0, 0x80F5, 0x80FF,	0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
	0x00A0, 0x80A5,	0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
	0x8093,	0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
	0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197,	0x0192,
	0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE,	0x01A4, 0x81A1,
	0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB,	0x01FE, 0x01F4, 0x81F1,
	0x81D3, 0x01D6, 0x01DC, 0x81D9,	0x01C8, 0x81CD, 0x81C7, 0x01C2,
	0x0140, 0x8145, 0x814F,	0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
	0x8173, 0x0176,	0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
	0x8123,	0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
	0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104,	0x8101,
	0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D,	0x8317, 0x0312,
	0x0330, 0x8335, 0x833F, 0x033A, 0x832B,	0x032E, 0x0324, 0x8321,
	0x0360, 0x8365, 0x836F, 0x036A,	0x837B, 0x037E, 0x0374, 0x8371,
	0x8353, 0x0356, 0x035C,	0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
	0x03C0, 0x83C5,	0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
	0x83F3,	0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
	0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7,	0x03B2,
	0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E,	0x0384, 0x8381,
	0x0280, 0x8285, 0x828F, 0x028A, 0x829B,	0x029E, 0x0294, 0x8291,
	0x82B3, 0x02B6, 0x02BC, 0x82B9,	0x02A8, 0x82AD, 0x82A7, 0x02A2,
	0x82E3, 0x02E6, 0x02EC,	0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
	0x02D0, 0x82D5,	0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
	0x8243,	0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
	0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264,	0x8261,
	0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E,	0x0234, 0x8231,
	0x8213, 0x0216, 0x021C, 0x8219, 0x0208,	0x820D, 0x8207, 0x0202
};

static struct dynamixel_port *dynamixel_ports = 0;
static size_t num_dynamixel_ports = 0;
static osOnceFlag_t dynamixel_port_init_flag = osOnceFlagsInit;

static uint16_t crc16(uint16_t crc, uint8_t *data, size_t size)
{
	uint16_t i;
	for (uint16_t j = 0; j < size; j++)
	{
		i = ((uint16_t)(crc >> 8) ^ *data++) & 0xFF;
		crc = (crc << 8) ^ crc_table[i];
	}

	return crc;
}

static size_t add_stuffing(uint8_t *data, size_t size)
{
	assert(data != 0);

	/* Search for 0xff, 0xff, 0xfd, looking backward */
	for (size_t i = 2; i < size; ++i)

		/* Stuff? */
		if (data[i - 2] == 0xff && data[i - 1] == 0xff && data[i] == 0xfd) {

			/* Make the hole and stuff */
			memmove(&data[i + 2], &data[i + 1], size - i);
			data[i + 1] = 0xfd;

			/* Increase the packet size */
			++size;
		}

	/* Will have the updated size */
	return size;
}

static size_t remove_stuffing(uint8_t *data, size_t size)
{
	assert(data != 0);

	/* Search for 0xff, 0xff, 0xfd, 0xfd */
	for (size_t i = 3; i < size; ++i)

		/* Remove stuffing? */
		if (data[i - 3] == 0xff && data[i - 2] == 0xff && data[i - 1] == 0xfd && data[i] == 0xfd) {

			/* Copy over stuffing */
			memmove(&data[i], &data[i + 1], size - i - 1);

			/* Reduce size */
			--size;
		}

	/* Will have the updated size */
	return size;
}

int dynamixel_port_tx_packet(struct dynamixel_port *port, struct dynamixel_packet *packet)
{
	int status = 0;

	assert(port != 0 && packet != 0);

	/* Initialize he header */
	packet->header.preamble[0] = 0xff;
	packet->header.preamble[1] = 0xff;
	packet->header.preamble[2] = 0xfd;
	packet->header.preamble[3] = 0x00;

	/* Add any required stuffing */
	packet->header.length = add_stuffing(packet->params, packet->header.length) + 2;

	/* Add the CRC-16 */
	uint16_t crc = crc16(0, (uint8_t *)packet, packet->header.length + sizeof(struct dynamixel_header) - 2);
	memcpy(packet->params + packet->header.length - 2, &crc, 2);

	/* Send the packet */
	if (half_duplex_send(port->hd, packet, packet->header.length + sizeof(struct dynamixel_header)) != packet->header.length + sizeof(struct dynamixel_header)) {
		errno = EIO;
		status = -errno;
	}

	/* Mostly good */
	return status;
}

int dynamixel_port_rx_packet(struct dynamixel_port *port, struct dynamixel_packet *packet, unsigned int timeout)
{
	assert(port != 0 && packet != 0);

	/* Receive a packet */
	int status = half_duplex_recv(port->hd, packet, packet->header.length + sizeof(struct dynamixel_header) + 2, timeout);
	if (status <= 0) {
		if (status == 0) {
			errno = ETIMEDOUT;
			status = -errno;
		}
		return status;
	}

	/* Range check the packet ID */
	if (packet->header.packet_id > DYNAMIXEL_MAX_PACKET_ID) {
		errno = EIO;
		return -errno;
	}

	/* Check the CRC */
	uint16_t crc;
	memcpy(&crc, packet->params + packet->header.length - 2, 2);
	if (crc != crc16(0, (uint8_t *)packet, packet->header.length + sizeof(struct dynamixel_header) - 2)) {
		errno = EDCRC;
		return -errno;
	}

	/* Remove any stuffing and update the packet length */
	packet->header.length = remove_stuffing(packet->params, packet->header.length - 2);

	/* All good */
	return 0;
}

int dynamixel_port_transceive_packet(struct dynamixel_port *port, struct dynamixel_instr_packet *tx_packet, struct dynamixel_status_packet *rx_packet, unsigned int timeout)
{
	assert(port != 0 && tx_packet != 0 && rx_packet != 0);

	/* Send the instruction packet */
	int status = dynamixel_port_tx_packet(port, (struct dynamixel_packet *)tx_packet);
	if (status < 0)
		return status;

	/* Receive the status packet */
	status = dynamixel_port_rx_packet(port, (struct dynamixel_packet *)rx_packet, timeout);
	if (status < 0)
		return status;

	/* Check the instruction status */
	if (rx_packet->instr != DYNAMIXEL_STATUS_INSTR) {
		errno = EIO;
		return -errno;
	}

	/* Check error status */
	if (rx_packet->error != 0) {
		errno = DYNAMIXEL_ERRORS + rx_packet->error;
		return -errno;
	}

	/* Should be good*/
	return 0;
}

int dynamixel_port_ping(struct dynamixel_port *port, uint8_t id, struct dynamixel_info *info)
{
	assert(port != 0);

	/* Initialize the request */
	struct dynamixel_ping_request request;
	request.header.packet_id = id;
	request.header.length = dynamixel_sizeof(request);
	request.instr.instruction = DYNAMIXEL_PING_INSTR;

	/* Initialize response */
	struct dynamixel_ping_response response;
	response.header.length = dynamixel_sizeof(response);

	/* Look for the provided id */
	int status = dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)&request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
	if (status < 0)
		return status;

	/* Save the ping response */
	info->id = response.header.packet_id;
	info->model = response.model;
	info->version = response.version;

	return 0;
}

int dynamixel_port_reset(struct dynamixel_port *port, uint8_t id, enum dynamixel_reset_cmd reset_cmd)
{
	assert(port != 0);

	/* Initialize the request */
	struct dynamixel_reset_request request;
	request.header.packet_id = id;
	request.header.length = dynamixel_sizeof(request);
	request.instr.instruction = DYNAMIXEL_FACTORY_RESET_INSTR;

	/* Initialize the response */
	struct dynamixel_reset_response response;
	response.header.length = dynamixel_sizeof(response);

	/* Lets go */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)&request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_reboot(struct dynamixel_port *port, uint8_t id)
{
	assert(port != 0);

	/* Initialize the request */
	struct dynamixel_reboot_request request;
	request.header.packet_id = id;
	request.header.length = dynamixel_sizeof(request);
	request.instr.instruction = DYNAMIXEL_REBOOT_INSTR;

	/* Initialize the response */
	struct dynamixel_reboot_response response;
	response.header.length = dynamixel_sizeof(response);

	/* Lets go */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)&request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_clear(struct dynamixel_port *port, uint8_t id, enum dynamixel_clear_cmd clear_cmd)
{
	assert(port != 0);

	/* Initialize the request */
	struct dynamixel_clear_request request;
	request.header.packet_id = id;
	request.header.length = dynamixel_sizeof(request);
	request.instr.instruction = DYNAMIXEL_FACTORY_RESET_INSTR;
	request.data[0] = clear_cmd;
	request.data[1] = 0x44;
	request.data[2] = 0x58;
	request.data[3] = 0x4c;
	request.data[4] = 0x22;

	/* Initialize the response */
	struct dynamixel_clear_response response;
	response.header.length = dynamixel_sizeof(response);

	/* Lets go */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)&request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_backup(struct dynamixel_port *port, uint8_t id, enum dynamixel_backup_cmd backup_cmd)
{
	assert(port != 0);

	/* Initialize the request */
	struct dynamixel_clear_request request;
	request.header.packet_id = id;
	request.header.length = dynamixel_sizeof(request);
	request.instr.instruction = DYNAMIXEL_FACTORY_RESET_INSTR;
	request.data[0] = backup_cmd;
	request.data[1] = 0x43;
	request.data[2] = 0x54;
	request.data[3] = 0x52;
	request.data[4] = 0x4c;

	/* Initialize the response */
	struct dynamixel_clear_response response;
	response.header.length = dynamixel_sizeof(response);

	/* Lets go */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)&request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_read(struct dynamixel_port *port, uint8_t id, uint16_t addr, void *buffer, size_t count)
{
	assert(port != 0 && buffer != 0);

	/* Initialize the request */
	struct dynamixel_read_request request;
	request.header.packet_id = id;
	request.header.length = dynamixel_sizeof(struct dynamixel_read_request);
	request.instr.instruction = DYNAMIXEL_READ_INSTR;
	request.address = addr;
	request.length = count;

	/* Initialize the response */
	struct dynamixel_read_response *response = (struct dynamixel_read_response *)port->buffer;
	response->header.length = dynamixel_sizeof(struct dynamixel_read_response) + count;

	/* Do it */
	int status = dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)&request, (struct dynamixel_status_packet *)response, DYNAMIXEL_TIMEOUT_MSEC);
	if (status < 0)
		return status;

	/* Copy the data TODO can we set a zero copy? */
	memcpy(buffer, response->params, count);

	/* All good */
	return status;
}

int dynamixel_port_write(struct dynamixel_port *port, uint8_t id, uint16_t addr, const void *buffer, size_t count)
{
	assert(port != 0 && buffer != 0);

	/* Initialize the request */
	struct dynamixel_write_request *request = (struct dynamixel_write_request *)port->buffer;
	request->header.packet_id = id;
	request->header.length = dynamixel_sizeof(struct dynamixel_write_request) + count;
	request->instr.instruction = DYNAMIXEL_WRITE_INSTR;
	request->address = addr;
	memcpy(request->params, buffer, count);

	/* Initialize the response */
	struct dynamixel_write_response response;
	response.header.length = dynamixel_sizeof(struct dynamixel_write_response);

	/* Do it */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_reg_write(struct dynamixel_port *port, uint8_t id, uint16_t addr, const void *buffer, size_t count)
{
	assert(port != 0 && buffer != 0);

	/* Initialize the request */
	struct dynamixel_write_request *request = (struct dynamixel_write_request *)port->buffer;
	request->header.packet_id = id;
	request->header.length = dynamixel_sizeof(*request) + count;
	request->instr.instruction = DYNAMIXEL_REG_WRITE_INSTR;
	request->address = addr;
	memcpy(request->params, buffer, count);

	/* Initialize the response */
	struct dynamixel_write_response response;
	response.header.length = dynamixel_sizeof(response);

	/* Do it */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_action(struct dynamixel_port *port, uint8_t id)
{
	assert(port != 0);

	/* Initialize the request */
	struct dynamixel_action_request request;
	request.header.packet_id = id;
	request.header.length = dynamixel_sizeof(request);
	request.instr.instruction = DYNAMIXEL_ACTION_INSTR;

	/* Initialize the response */
	struct dynamixel_action_response response;
	response.header.length = dynamixel_sizeof(response);

	/* Lets go */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)&request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_sync_read(struct dynamixel_port *port, uint8_t *ids, size_t num_ids, uint16_t addr, uint16_t length, void *buffer)
{
	assert(port != 0 && ids != 0 && buffer != 0);

	struct
	{
		uint8_t error;
		uint8_t id;
		uint8_t bytes[length];
		uint16_t crc;
	} __packed *data_buffer;

	/* Initialize the request */
	struct dynamixel_sync_read_request *request = (struct dynamixel_sync_read_request *)port->buffer;
	request->header.packet_id = DYNAMIXEL_BROADCAST_ID;
	request->header.length = dynamixel_sizeof(struct dynamixel_sync_read_request) + num_ids;
	request->instr.instruction = DYNAMIXEL_SYNC_READ_INSTR;
	request->addr = addr;
	request->length = length;
	memcpy(request->ids, ids, num_ids);

	/* Send the request */
	int status = dynamixel_port_tx_packet(port, (struct dynamixel_packet *)request);
	if (status < 0)
		return status;

	/* Initialize the response packet */
	struct dynamixel_fast_sync_read_response *response = (struct dynamixel_fast_sync_read_response *)port->buffer;
	response->header.length = dynamixel_sizeof(struct dynamixel_fast_sync_read_response) + sizeof(*data_buffer) * num_ids;

	/* Receive the response */
	status = dynamixel_port_rx_packet(port, (struct dynamixel_packet *)response, DYNAMIXEL_TIMEOUT_MSEC);
	if (status < 0)
		return status;

	/* Check the instruction status */
	if (response->instr != DYNAMIXEL_STATUS_INSTR) {
		errno = EIO;
		return -errno;
	}

	/* Unmarshal the data */
	data_buffer = (void *)response->data;
	uint8_t *buffer_pos = buffer;
	for (size_t i = 0; i < num_ids; ++i) {

		/* Check for an error */
		if (data_buffer[i].error != 0) {
			errno = DYNAMIXEL_ERRORS + data_buffer[i].error;
			return -errno;
		}

		/* Copy the data */
		memcpy(buffer_pos, data_buffer[i].bytes, length);

		/* Update the buffer position */
		buffer_pos += length;
	}

	/* All great */
	return 0;
}

int dynamixel_port_sync_write(struct dynamixel_port *port, uint8_t *ids, size_t num_ids, uint16_t addr, uint16_t length, const void *buffer)
{
	assert(port != 0 && ids != 0 && buffer != 0);

	struct
	{
		uint8_t id;
		uint8_t bytes[length];
	} __packed *data_buffer;

	/* Initialize the request */
	struct dynamixel_sync_write_request *request = (struct dynamixel_sync_write_request *)port->buffer;
	request->header.packet_id = DYNAMIXEL_BROADCAST_ID;
	request->header.length = dynamixel_sizeof(struct dynamixel_sync_write_request) + sizeof(*data_buffer) * num_ids;

	/* Marshal up the data */
	const uint8_t *buffer_pos = buffer;
	data_buffer = (void *)request->data;
	for(size_t i = 0 ; i < num_ids; ++i) {
		data_buffer[i].id = ids[i];
		memcpy(data_buffer[i].bytes, buffer_pos, length);
		buffer_pos += length;
	}

	/* Initialize the response */
	struct dynamixel_sync_write_response response;
	response.header.length = dynamixel_sizeof(struct dynamixel_sync_write_response);

	/* Send it off */
	return dynamixel_port_transceive_packet(port, (struct dynamixel_instr_packet *)request, (struct dynamixel_status_packet *)&response, DYNAMIXEL_TIMEOUT_MSEC);
}

int dynamixel_port_ini(struct dynamixel_port *port, unsigned int channel, unsigned int baud_rate, unsigned int rx_idle_timeout)
{
	assert(port != 0);

	/* Basic initialization */
	memset(port, 0, sizeof(struct dynamixel_port));

	/* Get the half duplex channel */
	port->hd = device_get_half_duplex_channel(channel);
	if (!port->hd)
		return -errno;

	/* Configure baud rate and timeout */
	int status = half_duplex_configure(port->hd, baud_rate, rx_idle_timeout);
	if (status < 0)
		return status;

	/* All good */
	return 0;
}

void dynamixel_port_fini(struct dynamixel_port *port)
{
}

struct dynamixel_port *dynamixel_port_create(unsigned int channel, unsigned int baud_rate, unsigned int rx_idle_timeout)
{
	/* Need memory */
	struct dynamixel_port *port = malloc(sizeof(struct dynamixel_port));
	if (!port)
		return 0;

	/* Initialize */
	int status = dynamixel_port_ini(port, channel, baud_rate, rx_idle_timeout);
	if (status < 0)
		free(port);

	return port;
}

void dynamixel_port_destroy(struct dynamixel_port *port)
{
	assert(port != 0);

	/* Forward */
	dynamixel_port_fini(port);

	/* And release */
	free(port);
}

static void dynamixel_port_instance_init(void)
{
	unsigned int port_channels[] = DYNAMIXEL_PORTS_HALF_DUPLEX_CHANNELS;

	/* Allocate ports, abort on failure */
	dynamixel_ports = calloc(array_sizeof(port_channels), sizeof(struct dynamixel_port));
	if (!dynamixel_ports)
		syslog_fatal("could not allocate dynamixel_ports");
	num_dynamixel_ports = array_sizeof(port_channels);

	/* Initialize the dynamixel ports */
	for (size_t i = 0; i < num_dynamixel_ports; ++i) {
		int status = dynamixel_port_ini(&dynamixel_ports[i], port_channels[i], DYNAMIXEL_PORTS_BAUD_RATE, DYNAMIXEL_PORTS_RX_IDLE_TIMEOUT);
		if (status < 0)
			syslog_fatal("failed to initialize dynamixel port %u: %d\n", i, status);
	}

	syslog_info("initialized %u dynamixel ports\n", num_dynamixel_ports);
}

size_t dynamixel_port_get_num_ports(void)
{
	return num_dynamixel_ports;
}

struct dynamixel_port *dynamixel_port_get_instance(unsigned int id)
{
	/* Initialize the singleton */
	osCallOnce(&dynamixel_port_init_flag, dynamixel_port_instance_init);

	if (id >= num_dynamixel_ports) {
		errno = ENODEV;
		return 0;
	}

	/* Return the port */
	return &dynamixel_ports[id];
}
