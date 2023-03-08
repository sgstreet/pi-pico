/*
 * component.h
 *
 *  Created on: Feb 17, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _COMPONENT_H_
#define _COMPONENT_H_

#include <stddef.h>

struct component
{
	const char *name;
	unsigned int priority;
	int (*ini)(void *component);
	void (*fini)(void *component);
};

void *component_find(const char *name);

#endif
