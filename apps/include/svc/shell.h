/*
 * shell.h
 *
 *  Created on: Feb 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SHELL_H_
#define _SHELL_H_

#include <compiler.h>

#define __shell_command __unused __section(".shell_cmd")

struct shell_command
{
	const char *name;
	const char *usage;
	int (*func)(int argc, char **argv);
};

#endif
