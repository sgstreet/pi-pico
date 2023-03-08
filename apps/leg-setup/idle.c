/*
 * idle.c
 *
 *  Created on: Feb 25, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <rtos/rtos.h>

static void idle_task(void *context)
{
	while (true);
}

static __constructor void idle_ini(void)
{
	osThreadAttr_t idle_attr = { .name = "idle-task", .stack_size = 512, .priority = osPriorityIdle, .attr_bits = osThreadDetached };
	osThreadNew(idle_task, 0, &idle_attr);
}
