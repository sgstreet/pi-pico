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
void save_fault(const struct cortexm_fault *fault);

int stdout_putchar(int c)
{
	return diag_putc(c);
}

void *_rtos2_alloc(size_t size)
{
	return calloc(1, size);
}

void _rtos2_release(void *ptr)
{
	free(ptr);
}

core_local struct backtrace fault_backtrace[10];
core_local char fault_buffer[256];

void save_fault(const struct cortexm_fault *fault)
{
	backtrace_frame_t backtrace_frame;
	char *buffer = cls_datum(fault_buffer);
	backtrace_t *backtrace = cls_datum(fault_backtrace);

	/* Setup for a backtrace */
	backtrace_frame.fp = fault->r7;
	backtrace_frame.lr = fault->LR;
	backtrace_frame.sp = fault->SP;

	/* I'm not convinced this is correct,  */
	backtrace_frame.pc = fault->exception_return == 0xfffffff1 ? fault->LR : fault->PC;

	/* Try the unwind */
	int backtrace_entries = _backtrace_unwind(backtrace, array_sizeof(fault_backtrace), &backtrace_frame);

	/* Down via the board IO function */
	for (size_t i = 0; i < backtrace_entries; ++i) {
		snprintf(buffer, 256, "%s@%p - %p", backtrace[i].name, backtrace[i].function, backtrace[i].address);
		buffer[255] = 0;
		board_puts(buffer);
	}
}

int main(int argc, char **argv)
{
	return cmsis_rv2();
}
