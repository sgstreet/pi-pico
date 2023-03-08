/*
 * leg-manager.c
 *
 *  Created on: Feb 25, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdlib.h>

#include <devices/devices.h>

#include "leg-manager.h"

int leg_manager_ini(struct leg_manager *manager, size_t num_legs, unsigned int baud_rate, unsigned int idle_timeout)
{
	assert(manager != 0);

	/* Allocate the leg ports */
	manager->legs = calloc(num_legs, sizeof(struct dynamixel_port));
	if (!manager->legs)
		return -errno;

	/* Basic initialization */
	manager->num_legs = num_legs;

	/* Initialize each leg port */
	for (size_t i = 0; i < manager->num_legs; ++i) {
		int status = dynamixel_port_ini(&manager->legs[i], i, baud_rate, idle_timeout);
		if (status < 0) {
			free(manager->legs);
			return status;
		}
	}

	/* All good */
	return 0;
}

void leg_manager_fini(struct leg_manager *manager)
{
	assert(manager != 0);

	for (size_t i = 0; i < manager->num_legs; ++i)
		dynamixel_port_fini(&manager->legs[i]);
	free(manager->legs);
}

struct leg_manager *leg_manager_create(size_t num_legs, unsigned int baud_rate, unsigned int idle_timeout)
{
	/* Allocate the manager */
	struct leg_manager *manager = malloc(sizeof(struct leg_manager));
	if (!manager)
		return 0;

	/* Forward */
	int status = leg_manager_ini(manager, num_legs, baud_rate, idle_timeout);
	if (status < 0) {
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
