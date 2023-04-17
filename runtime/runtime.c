/*
 * runtime.c
 *
 *  Created on: Apr 15, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>

#include <init/init-sections.h>
#include <diag/diag.h>

extern void __aeabi_mem_init(void);
extern void __aeabi_bits_init(void);
extern void __aeabi_float_init(void);
extern void __aeabi_double_init(void);

__attribute__((format(printf, 1, 2))) void panic(const char *fmt, ...);

__noreturn void panic(const char *fmt, ...)
{
	va_list ap;

	/* Initialize the variable arguments */
	va_start(ap, fmt);

	diag_vprintf(fmt, ap);

	/* Clean up */
	va_end(ap);

	abort();
}

static void runtime_init(void)
{
	__aeabi_mem_init();
	__aeabi_bits_init();
	__aeabi_float_init();
	__aeabi_double_init();
}
PREINIT_SYSINIT_WITH_PRIORITY(runtime_init, 005);
