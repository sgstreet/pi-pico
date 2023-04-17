/*
 * leg-manager.h
 *
 *  Created on: Feb 25, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _LEG_MANAGER_H_
#define _LEG_MANAGER_H_

#include <assert.h>
#include <stdint.h>

#include <legs/leg.h>

struct leg_manager
{
	size_t num_legs;
	struct leg legs[];
};

int leg_manager_ini(struct leg_manager *manager, size_t num_legs, unsigned int *leg_ports);
void leg_manager_fini(struct leg_manager *manager);

struct leg_manager *leg_manager_create(size_t num_legs, unsigned int *leg_ports);
void leg_manager_destroy(struct leg_manager *manager);

struct leg_manager *leg_manager_get_instance(void);

static inline struct leg *leg_manager_get_leg(struct leg_manager *manager, unsigned int leg)
{
	assert(manager != 0);

	if (leg < manager->num_legs)
		return &manager->legs[leg];

	return 0;
}

#endif
