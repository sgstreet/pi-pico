/*
 * leg_cmds.c
 *
 *  Created on: Feb 26, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <svc/shell.h>

#include "leg-manager.h"

extern struct leg_manager *leg_manager;

static int ping_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	/* Do we have at least a leg arg? */
	if (argc < 2) {
		fprintf(stderr, "missing parameters\n");
		return EXIT_FAILURE;
	}

	/* Convert the port */
	errno = 0;
	char *end;
	unsigned int leg = strtoul(argv[1], &end, 0);
	if (errno != 0 || end == argv[1] || leg > HALF_DUPLEX_NUM_CHANNELS) {
		printf("bad leg: '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* Convert option id, default to 254 for broadcast ping */
	unsigned int id = 254;
	if (argc > 2) {
		errno = 0;
		id = strtoul(argv[2], &end, 0);
		if (errno != 0 || end == argv[2] || id > 254) {
			printf("bad id: '%s'\n", argv[2]);
			return EXIT_FAILURE;
		}
	}

	/* Get the dynamixel port */
	struct dynamixel_port *leg_port = leg_manager_get_port(leg_manager, leg);
	if (!leg_port) {
		fprintf(stderr, "failed to get the dynamixel port for %u: %d", leg, errno);
		return EXIT_FAILURE;
	}

	/* Ping the port */
	struct dynamixel_info info[64];
	int count = dynamixel_ping(leg_port, id, info, 64);
	if (count < 0) {
		fprintf(stderr, "failed to ping leg port %u with id %u: %d\n", leg, id, errno);
		dynamixel_port_destroy(leg_port);
		return EXIT_FAILURE;
	}

	/* Dump the info */
	for (int i = 0; i < count; ++i)
		printf("found servo at %u, model: %04x, version: %02x\n", info[i].id, info[i].model, info[i].version);

	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command _ping_cmd =
{
	.name = "ping",
	.usage = "<leg> [id]",
	.func = ping_cmd,
};

static int read_item_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	union {
		uint8_t byte;
		uint16_t half_word;
		uint32_t word;
	} value;

	/* Do we have enough arguments? */
	if (argc < 5) {
		fprintf(stderr, "missing parameters\n");
		return EXIT_FAILURE;
	}

	/* Convert the port */
	errno = 0;
	char *end;
	unsigned int leg = strtoul(argv[1], &end, 0);
	if (errno != 0 || end == argv[1] || leg > HALF_DUPLEX_NUM_CHANNELS) {
		printf("bad leg: '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* Convert option id, default to 254 for broadcast ping */
	errno = 0;
	unsigned int id = strtoul(argv[2], &end, 0);
	if (errno != 0 || end == argv[2] || id > 254) {
		printf("bad id: '%s'\n", argv[2]);
		return EXIT_FAILURE;
	}

	unsigned int addr = strtoul(argv[3], &end, 0);
	if (errno != 0 || end == argv[3]) {
		printf("bad addr: '%s'\n", argv[3]);
		return EXIT_FAILURE;
	}

	unsigned int size = strtoul(argv[4], &end, 0);
	if (errno != 0 || end == argv[4] || (size != 1 && size != 2 && size != 4)) {
		printf("bad size: '%s'\n", argv[4]);
		return EXIT_FAILURE;
	}

	/* Get the dynamixel port */
	struct dynamixel_port *leg_port = leg_manager_get_port(leg_manager, leg);
	if (!leg_port) {
		fprintf(stderr, "failed to get the dynamixel port for %u: %d", leg, errno);
		return EXIT_FAILURE;
	}

	/* Read the value */
	int status = dynamixel_read(leg_port, id, addr, &value, size);
	if (status < 0) {
		fprintf(stderr, "failed to read %u bytes from id %u at addr %u: %d\n", size, id, addr, status);
		return EXIT_FAILURE;
	}

	/* Dump the value */
	switch (size) {
		case 1:
			printf("0x%02hhx\n", value.byte);
			break;
		case 2:
			printf("0x%04hx\n", value.half_word);
			break;
		case 4:
			printf("0x%08lx\n", value.word);
			break;
	}

	/* All good */
	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command _read_item_cmd =
{
	.name = "read-item",
	.usage = "<leg> <id> <addr> <[ 1 | 2 | 4 ]>",
	.func = read_item_cmd,
};

static int write_item_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	union {
		uint8_t byte;
		uint16_t half_word;
		uint32_t word;
	} value;

	/* Do we have enough arguments? */
	if (argc < 6) {
		fprintf(stderr, "missing parameters\n");
		return EXIT_FAILURE;
	}

	/* Convert the port */
	errno = 0;
	char *end;
	unsigned int leg = strtoul(argv[1], &end, 0);
	if (errno != 0 || end == argv[1] || leg > HALF_DUPLEX_NUM_CHANNELS) {
		printf("bad leg: '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* Convert option id, default to 254 for broadcast ping */
	errno = 0;
	unsigned int id = strtoul(argv[2], &end, 0);
	if (errno != 0 || end == argv[2] || id > 254) {
		printf("bad id: '%s'\n", argv[2]);
		return EXIT_FAILURE;
	}

	/* Convert address */
	unsigned int addr = strtoul(argv[3], &end, 0);
	if (errno != 0 || end == argv[3]) {
		printf("bad addr: '%s'\n", argv[3]);
		return EXIT_FAILURE;
	}

	/* Convert the size */
	unsigned int size = strtoul(argv[4], &end, 0);
	if (errno != 0 || end == argv[4] || (size != 1 && size != 2 && size != 4)) {
		printf("bad size: '%s'\n", argv[4]);
		return EXIT_FAILURE;
	}

	/* Convert the value */
	value.word = strtoul(argv[5], &end, 0);
	if (errno != 0 || end == argv[5]) {
		printf("bad value: '%s'\n", argv[4]);
		return EXIT_FAILURE;
	}

	/* Adjust to sizes less than 4 */
	switch (size) {
		case 1:
			value.byte = value.word & 0xff;
			break;
		case 2:
			value.half_word = value.word & 0xffff;
			break;
	}

	/* Get the dynamixel port */
	struct dynamixel_port *leg_port = leg_manager_get_port(leg_manager, leg);
	if (!leg_port) {
		fprintf(stderr, "failed to get the dynamixel port for %u: %d", leg, errno);
		return EXIT_FAILURE;
	}

	/* Write the value */
	int status = dynamixel_write(leg_port, id, addr, &value, size);
	if (status < 0) {
		fprintf(stderr, "failed to write %u bytes from id %u at addr %u: %d\n", size, id, addr, status);
		return EXIT_FAILURE;
	}

	/* All good by this point */
	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command _write_item_cmd =
{
	.name = "write-item",
	.usage = "<leg> <id> <addr> <[ 1 | 2 | 4 ]> <value>",
	.func = write_item_cmd,
};
