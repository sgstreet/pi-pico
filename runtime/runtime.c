/*
 * runtime.c
 *
 *  Created on: Apr 15, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdlib.h>
#include <compiler.h>

#include <diag/diag.h>

__noreturn void panic(const char *fmt, ...);

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
