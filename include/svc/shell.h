/*
 * shell.h
 *
 *  Created on: Feb 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _SHELL_H_
#define _SHELL_H_

#include <compiler.h>
#include <svc/svc.h>

#ifndef SHELL_COMMAND_STACK_SIZE
#define SHELL_COMMAND_STACK_SIZE 2048
#endif

#ifndef SHELL_PROMPT
#define SHELL_PROMPT "shell> "
#endif

#ifndef SHELL_MOD
#define SHELL_MOD "\nWelcome to the shell\n"
#endif

#define __shell_command __used __section(".shell_cmd")

struct shell_command
{
	const char *name;
	const char *usage;
	int (*func)(int argc, char **argv);
};

#endif
