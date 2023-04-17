/*
 * leg-manager.c
 *
 *  Created on: Feb 25, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/syslog.h>
#include <rtos/rtos.h>

#include <legs/leg-manager.h>

static struct leg_manager *leg_manager_instance = 0;
static osOnceFlag_t leg_manager_init_flag = osOnceFlagsInit;

int leg_manager_ini(struct leg_manager *manager, size_t num_legs, unsigned int *leg_ports)
{
	int status;

	assert(manager != 0 && leg_ports != 0);

	/* Clear the manager */
	memset(manager, 0, sizeof(*manager) + num_legs * sizeof(struct leg));

	/* Initialize the legs */
	for (size_t i = 0; i < num_legs; ++i) {
		status = leg_ini(&manager->legs[i], leg_ports[i]);
		if (status < 0) {
			syslog_error("failed_to initialize leg %u: %d\n", i, status);
			goto error_cleanup_legs;
		}
		++manager->num_legs;
	}

	/* All good */
	return 0;

error_cleanup_legs:

	/* Clean up all initialized legs */
	for (size_t i = 0; i < manager->num_legs; ++i)
		leg_fini(&manager->legs[i]);

	/* Bad, very bad */
	return status;
}

void leg_manager_fini(struct leg_manager *manager)
{
	assert(manager != 0);

	/* Clean up all legs */
	for (size_t i = 0; i < manager->num_legs; ++i)
		leg_fini(&manager->legs[i]);
}

struct leg_manager *leg_manager_create(size_t num_legs, unsigned int *leg_ports)
{
	/* Allocate the manager */
	struct leg_manager *manager = malloc(sizeof(struct leg_manager) + num_legs * sizeof(struct leg));;
	if (!manager) {
		syslog_error("failed to allocate a leg manager with %u legs\n", num_legs);
		return 0;
	}

	/* Forward */
	int status = leg_manager_ini(manager, num_legs, leg_ports);
	if (status < 0) {
		syslog_error("failed to initialize the leg manager: %d\n", status);
		free(manager);
		return 0;
	}

	/* All good */
	return manager;
}

void leg_manager_destroy(struct leg_manager *manager)
{
	assert(manager != 0);

	/* Forward and release */
	leg_manager_fini(manager);
	free(manager);
}

static void leg_manager_instance_init(void)
{
	unsigned int leg_ports[] = LEG_MANAGER_LEG_DYNAMIXEL_PORTS;

	/* Create the singleton */
	leg_manager_instance = leg_manager_create(array_sizeof(leg_ports), leg_ports);
	if (!leg_manager_instance)
		syslog_fatal("failed to create the leg manager: %d\n", errno);

	/* Log initialization */
	syslog_info("initialized with %u legs\n", leg_manager_instance->num_legs);
}

struct leg_manager *leg_manager_get_instance(void)
{
	/* Singleton initialization */
	osCallOnce(&leg_manager_init_flag, leg_manager_instance_init);

	/* Let them have it */
	return leg_manager_instance;
}
