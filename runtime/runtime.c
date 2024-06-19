/*
 * runtime.c
 *
 *  Created on: Apr 15, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <compiler.h>

#include <diag/diag.h>

extern void __atomic_init(void);
extern void __aeabi_mem_init(void);
extern void __aeabi_bits_init(void);
extern void __aeabi_float_init(void);
extern void __aeabi_double_init(void);
extern void __cls_tls_init(void);

__noreturn void panic(const char *fmt, ...);
void runtime_init(void);

void panic(const char *fmt, ...)
{
	va_list ap;

	/* Initialize the variable arguments */
	va_start(ap, fmt);

	diag_vprintf(fmt, ap);

	/* Clean up */
	va_end(ap);

	abort();
}

void runtime_init(void)
{
	__aeabi_mem_init();
	__aeabi_bits_init();
	__aeabi_float_init();
	__aeabi_double_init();
	__atomic_init();
}
