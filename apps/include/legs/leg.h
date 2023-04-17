/*
 * leg.h
 *
 *  Created on: Apr 6, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _LEG_H_
#define _LEG_H_

#include <stdarg.h>

#include <dynamixel/dynamixel.h>

#define LEG_RETURN_DELAY_TIME 250U
#define LEG_DRIVE_MODE 0U
#define LEG_OPERATING_MODE 3U
#define LEG_TEMPERATURE_LIMIT 70U
#define LEG_MAX_VOLTAGE_LIMIT 55U
#define LEG_MIN_VOLTAGE_LIMIT 45U
#define LEG_CURRENT_LIMIT 1000U
#define LEG_VELOCITY_LIMIT 445U
#define LEG_MAX_POSITION_LIMIT 2731U
#define LEG_MIN_POSITION_LIMIT 1365U
#define LEG_SHUTDOWN 0X35U

struct leg_config
{
};

struct x
{
	uint32_t pos;
	uint32_t vel;
	uint32_t accel;
};

struct leg
{
	unsigned int id;
	unsigned int secondary_id;
	struct dynamixel_port *port;
};

int leg_ini(struct leg *leg, unsigned int id);
void leg_fini(struct leg *leg);

struct leg* leg_create(unsigned int id);
void leg_destroy(struct leg *leg);

int leg_for_each_joint(struct leg *leg, int (*joint_func)(struct leg *leg, unsigned int joint_id, va_list args), ...);

int leg_validate(struct leg *leg);
int leg_configure(struct leg *leg, struct leg_config *config);

int leg_home(struct leg *leg);

#endif
