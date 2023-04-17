/*
 * Copyright (C) 2017 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * startup.c
 *
 * Created on: Mar 16, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <alloca.h>

#include <signal.h>
#include <unistd.h>

#include <sys/types.h>

#include <compiler.h>
#include <init/init-sections.h>

extern void __initialize_args(int* argc, char***argv);
extern int _main(int argc, char* argv[]);
extern void _exit(int status) __noreturn;
extern void __assert_fail(const char *expr);
extern void _init(void);
extern void _fini(void);

extern int main(int argc, char **argv);
extern void _init_tls(void *__tls_block);
extern void _set_tls(void *tls);

void _start(void);
void abort(void);

char* __env[1] = { 0 };
char** environ = __env;

void *__tls_block = 0;

__weak void __initialize_args(int *argc, char***argv)
{
	static char __name[] = "";
	static char *__argv[2] = { __name, 0 };

	*argc = 1;
	*argv = &__argv[0];
}

__weak __noreturn void _exit(int status)
{
	while (true);
}

__weak __noreturn void abort(void)
{
	_exit(EXIT_FAILURE);
}

__weak __noreturn void __assert_fail(const char *expr)
{
	abort();
}

#if 0

void *sbrk(ptrdiff_t incr)
{
	static char *brk = __heap_start;

	if (incr < 0) {
		if (brk - __heap_start < -incr)
			return (void *)-1;
	} else {
		if (__heap_end - brk < incr)
			return (void *)-1;
	}

	void *block = brk;
	brk += incr;
	return block;
}


void __aeabi_init_tls(void *tls);
__weak void __aeabi_init_tls(void *tls)
{
	size_t tdata_size = (size_t)&__tdata_size;
	size_t tbss_size = (size_t)&__tbss_size;

	void *tdata = &__tdata;
	if (tdata_size > 0)
		memcpy(tls, tdata, tdata_size);

	tls += tdata_size;
	if (tbss_size > 0)
		memset(tls, 0, tbss_size);
}

void *__aeabi_read_tp(void);
__weak __naked void *__aeabi_read_tp(void)
{
	asm volatile (
		"ldr r0, =__tls_block \n"
		"ldr r0, [r0] \n"
		"mov pc, lr \n"
	);
}

void _init_tls(void *tls);
__weak void _init_tls(void *tls)
{
	size_t tdata_size = (size_t)&__tdata_size;
	size_t tbss_size = (size_t)&__tbss_size;

	void *tdata = &__tdata;
	if (tdata_size > 0)
		memcpy(tls, tdata, tdata_size);

	tls += tdata_size;
	if (tbss_size > 0)
		memset(tls, 0, tbss_size);
}

void _set_tls(void *tls);
__weak void _set_tls(void *tls)
{
	__tls_block = tls;
}
#endif

__weak void _init(void)
{
}

__weak void _fini(void)
{
}

__weak int _main(int argc, char **argv)
{
	/* Common init function */
	_init();

	/* Execute the ini array */
	run_init_array(__init_array_start, __init_array_end);

	int status = main(argc, argv);

	/* Execute the fini array */
	run_init_array(__fini_array_start, __fini_array_end);

	/* Common exit */
	_fini();

	/* All done */
	return status;
}

__isr_section __noreturn __weak void _start(void)
{
	int argc;
	char **argv;

	/* Execute the preinit array */
 	run_init_array(__preinit_array_start, __preinit_array_end);

 	/* Initialize the tls block */
	__tls_block = alloca((size_t)&__tls_size);
 	_init_tls(__tls_block);

 	/* Bind the initial TLS block */
 	_set_tls(__tls_block);

	/* Setup the program arguments */
	__initialize_args(&argc, &argv);

	/* Forward to the next stage */
	int status = _main(argc, argv);

	/* Forward to the _exit routine with the result, this bypasses the picolib calls to fini at  */
	exit(status);
}
