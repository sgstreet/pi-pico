#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fault.h>
#include <diag/diag.h>

#include "cmsis_rv2.h"

int stdout_putchar(int ch);
void *_rtos2_alloc(size_t size);
void _rtos2_release(void *ptr);
void save_fault(const struct cortexm_fault *fault, const struct backtrace *entries, int count);

int stdout_putchar(int c)
{
	return diag_putc(c);
}

void *_rtos2_alloc(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr)
		abort();
	memset(ptr, -1, size);
	return ptr;
}

void _rtos2_release(void *ptr)
{
	if (!ptr)
		abort();
	free(ptr);
}

void save_fault(const struct cortexm_fault *fault, const struct backtrace *backtrace, int count)
{
	fprintf(stderr, "faulted\n");
	for (int i = 0; i < count; ++i)
		fprintf(stderr, "%p - %s + %u\n", backtrace[i].address, backtrace[i].name, backtrace[i].address - backtrace[i].function);
}

int main(int argc, char **argv)
{
	return cmsis_rv2();
}
