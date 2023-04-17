/*
 * leg.c
 *
 *  Created on: Apr 6, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/syslog.h>

#include <legs/leg.h>
#include <dynamixel/dynamixel-xl330.h>

#define COXA_ID 11U
#define FEMUR_ID 12U
#define TIBA_ID 13U

#define RETURN_DELAY_TIME_VALUE 250U
#define DRIVE_MODE_VALUE 0L
#define OPERATING_MODE_VALUE 3U

struct leg_item
{
	uint8_t addr;
	size_t length;
	union {
		uint8_t data[4];
		uint8_t data1;
		uint16_t data2;
		uint32_t data4;
	};
};

int leg_ini(struct leg *leg, unsigned int id)
{
	assert(leg != 0);

	/* Save the leg id */
	leg->id = id;
	leg->secondary_id = id * 10 + 10;

	/* Try to get the matching dynamixel port */
	leg->port = dynamixel_port_get_instance(id);
	if (!leg->port) {
		syslog_error("failed to get dynamixel port %u: %d\n", id, errno);
		return -errno;
	}

	/* All good */
	return 0;
}

void leg_fini(struct leg *leg)
{
}

struct leg* leg_create(unsigned int id)
{
	/* Allocate the leg */
	struct leg *leg = calloc(1, sizeof(struct leg));
	if (!leg) {
		syslog_error("failed to allocate leg for id %u\n", id);
		return 0;
	}

	/* Forward */
	int status = leg_ini(leg, id);
	if (status < 0) {
		syslog_error("failed to initialize leg %u: %d\n", id, status);
		free(leg);
		return 0;
	}

	/* All find today */
	return leg;
}

void leg_destroy(struct leg *leg)
{
	assert(leg != 0);

	/* Forward */
	leg_fini(leg);

	/* Give it back */
	free(leg);
}

int leg_for_each_joint(struct leg *leg, int (*joint_func)(struct leg *leg, unsigned int joint_id, va_list args), ...)
{
	assert(joint_func != 0);

	int status = 0;

	va_list args;
	va_start(args, joint_func);

	/* Dispatch to joint handler */
	for (unsigned int i = COXA_ID; i <= TIBA_ID; ++i) {

		/* We quit when the status is not zero, or continue */
		status = joint_func(leg, i, args);
		if (status != 0)
			break;
	}

	va_end(args);

	/* All good */
	return status;
}

static int leg_validate_joint(struct leg *leg, unsigned int joint_id, va_list args)
{
	assert(leg != 0 && joint_id <= TIBA_ID);

	struct dynamixel_info info;
	struct dynamixel_info *expected = va_arg(args, struct dynamixel_info *);

	/* Execute ping command */
	int status = dynamixel_port_ping(leg->port, joint_id, &info);
	if (status < 0) {
		syslog_warn("failed to ping leg %u joint %u: %d\n", leg->id, joint_id, status);
		return joint_id;
	}

	/* Ensure we have the correct servo type */
	if (info.id != joint_id || info.model != expected->model || info.version != expected->version) {
		syslog_warn("leg %u joint %u servo has unexpected model or version found model: %04x with version %u\n", leg->id, joint_id, info.model, info.version);
		return joint_id;
	}

	/* Log the joy */
	syslog_info("found leg %u joint %u servo with model %04x and version %u\n", leg->id, joint_id, info.model, info.version);
	return 0;
}

int leg_validate(struct leg *leg)
{
	assert(leg != 0);

	struct dynamixel_info expected = { 0, 50, 0x04b0 };

	/* Make sure we can see each leg */
	int status = leg_for_each_joint(leg, leg_validate_joint, &expected);
	if (status != 0) {
		errno = EINVAL;
		return -errno;
	}

	/* All good */
	syslog_info("leg %u is valid\n", leg->id);
	return 0;
}

static int leg_write_joint_item(struct leg *leg, unsigned int joint_id, va_list args)
{
	assert(leg != 0 && joint_id <= TIBA_ID);

	struct leg_item *item = va_arg(args, struct leg_item *);

	/* Write the item to the joint */
	int status = dynamixel_port_write(leg->port, joint_id, item->addr, item->data, item->length);
	if (status < 0) {
		syslog_error("leg %u, joint %u, failed to set addr %u: %d\n", leg->id, joint_id, item->addr, status);
		return status;
	}

	return 0;
}

int leg_configure(struct leg *leg, struct leg_config *config)
{
	assert(leg != 0);

	/* Set the return delay time */
	syslog_info("leg %u, setting return delay time to %u\n", leg->id, LEG_RETURN_DELAY_TIME);
	struct leg_item return_delay_time = { .addr = RETURN_DELAY_TIME_ADDR, .length = RETURN_DELAY_TIME_SIZE, .data1 = LEG_RETURN_DELAY_TIME };
	int status = leg_for_each_joint(leg, leg_write_joint_item, &return_delay_time);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the secondary id */
	syslog_info("leg %u, setting secondary id to %u\n", leg->id, leg->id * 10 + 10);
	struct leg_item secondary_id = { .addr = SECONDARY_ID_ADDR, .length = SECONDARY_ID_SIZE, .data1 =  leg->id * 10 + 10};
	status = leg_for_each_joint(leg, leg_write_joint_item, &secondary_id);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the drive mode */
	syslog_info("leg %u, setting drive mode to 0x%02x\n", leg->id, LEG_DRIVE_MODE);
	struct leg_item drive_mode = { .addr = DRIVE_MODE_ADDR, .length = DRIVE_MODE_SIZE, .data1 = LEG_DRIVE_MODE};
	status = leg_for_each_joint(leg, leg_write_joint_item, &drive_mode);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the operating mode */
	syslog_info("leg %u, setting operating mode to %u\n", leg->id, LEG_OPERATING_MODE);
	struct leg_item operating_mode = { .addr = OPERATING_MODE_ADDR, .length = OPERATING_MODE_SIZE, .data1 = LEG_OPERATING_MODE};
	status = leg_for_each_joint(leg, leg_write_joint_item, &operating_mode);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the temperature limit */
	syslog_info("leg %u, setting temperature limit to %u\n", leg->id, LEG_TEMPERATURE_LIMIT);
	struct leg_item temperature_limit = { .addr = TEMPERATURE_LIMIT_ADDR, .length = TEMPERATURE_LIMIT_SIZE, .data1 = LEG_TEMPERATURE_LIMIT};
	status = leg_for_each_joint(leg, leg_write_joint_item, &temperature_limit);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the max voltage limit */
	syslog_info("leg %u, setting max voltage limit to %u\n", leg->id, LEG_MAX_VOLTAGE_LIMIT);
	struct leg_item max_voltage_limit = { .addr = MAX_VOLTAGE_LIMIT_ADDR, .length = MAX_VOLTAGE_LIMIT_SIZE, .data1 = LEG_MAX_VOLTAGE_LIMIT};
	status = leg_for_each_joint(leg, leg_write_joint_item, &max_voltage_limit);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the max voltage limit */
	syslog_info("leg %u, setting min voltage limit to %u\n", leg->id, LEG_MIN_VOLTAGE_LIMIT);
	struct leg_item min_voltage_limit = { .addr = MIN_VOLTAGE_LIMIT_ADDR, .length = MIN_VOLTAGE_LIMIT_SIZE, .data1 = LEG_MIN_VOLTAGE_LIMIT};
	status = leg_for_each_joint(leg, leg_write_joint_item, &min_voltage_limit);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the current limit */
	syslog_info("leg %u, setting current limit to %u\n", leg->id, LEG_CURRENT_LIMIT);
	struct leg_item current_limit = { .addr = CURRENT_LIMIT_ADDR, .length = CURRENT_LIMIT_SIZE, .data2 = LEG_CURRENT_LIMIT};
	status = leg_for_each_joint(leg, leg_write_joint_item, &current_limit);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the velocity limit */
	syslog_info("leg %u, setting velocity limit to %u\n", leg->id, LEG_VELOCITY_LIMIT);
	struct leg_item velocity_limit = { .addr = VELOCITY_LIMIT_ADDR, .length = VELOCITY_LIMIT_SIZE, .data2 = LEG_VELOCITY_LIMIT};
	status = leg_for_each_joint(leg, leg_write_joint_item, &velocity_limit);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the max position limit */
	syslog_info("leg %u, setting max position limit to %u\n", leg->id, LEG_MAX_POSITION_LIMIT);
	struct leg_item max_position_limit = { .addr = MAX_POSITION_LIMIT_ADDR, .length = MAX_POSITION_LIMIT_SIZE, .data4 = LEG_MAX_POSITION_LIMIT};
	status = leg_for_each_joint(leg, leg_write_joint_item, &max_position_limit);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the max position limit */
	syslog_info("leg %u, setting min position limit to %u\n", leg->id, LEG_MIN_POSITION_LIMIT);
	struct leg_item min_position_limit = { .addr = MIN_POSITION_LIMIT_ADDR, .length = MIN_POSITION_LIMIT_SIZE, .data4 = LEG_MIN_POSITION_LIMIT};
	status = leg_for_each_joint(leg, leg_write_joint_item, &min_position_limit);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* Set the drive mode */
	syslog_info("leg %u, setting shutdown to 0x%02x\n", leg->id, LEG_SHUTDOWN);
	struct leg_item shutdown = { .addr = SHUTDOWN_ADDR, .length = SHUTDOWN_SIZE, .data1 = LEG_SHUTDOWN };
	status = leg_for_each_joint(leg, leg_write_joint_item, &shutdown);
	if (status != 0) {
		errno = -status;
		return status;
	}

	/* All done at last */
	syslog_info("leg %u configured\n", leg->id);
	return 0;
}

int leg_home(struct leg *leg)
{
	errno = ENOTSUP;
	return -errno;
}
