/*
 * debug-cmds.c
 *
 *  Created on: Apr 26, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>

#include <cmsis/cmsis.h>

#include <svc/shell.h>

static int cpu_active_cmd(int argc, char **argv)
{
	printf("%lu\n", SIO->CPUID);
	return EXIT_SUCCESS;
}

static __shell_command const struct shell_command _cpu_active_cmd =
{
	.name = "cpu-active",
	.usage = "",
	.func = cpu_active_cmd,
};

static int mem_read_cmd(int argc, char **argv)
{
	return 0;
}

static __shell_command const struct shell_command _mem_read_cmd =
{
	.name = "mem-read",
	.usage = "TBD",
	.func = mem_read_cmd,
};

static int mem_write_cmd(int argc, char **argv)
{
	return 0;
}

static __shell_command const struct shell_command _mem_write_cmd =
{
	.name = "mem-write",
	.usage = "TBD",
	.func = mem_write_cmd,
};
