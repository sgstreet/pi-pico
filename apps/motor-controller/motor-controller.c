/*
 * motor-controller.c
 *
 *  Created on: Aug 17, 2024
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <diag/diag.h>
#include <init/init-sections.h>
#include <sys/iob.h>
#include <sys/syslog.h>
#include <sys/fault.h>
#include <sys/tls.h>

#include <rtos/rtos.h>

struct main_context
{
	int argc;
	char **argv;
	int status;
};

extern void _init(void);
extern void _fini(void);
int _main(int argc, char **argv);

struct iob old_stdio_iob;
struct iob old_stderr_iob;

core_local struct backtrace fault_backtrace[25];

static void dump_backtrace(const struct backtrace *backtrace, int count)
{
	char buffer[64];

	/* Dump via the board IO function */
	for (size_t i = 0; i < count; ++i) {
		snprintf(buffer, array_sizeof(buffer), "%s@%p - %p", backtrace[i].name, backtrace[i].function, backtrace[i].address);
		buffer[array_sizeof(buffer) - 1] = 0;
		board_puts(buffer);
	}
}

void save_fault(const struct cortexm_fault *fault)
{
	backtrace_frame_t backtrace_frame;
	backtrace_t *backtrace = cls_datum(fault_backtrace);

	/* Setup for a backtrace */
	backtrace_frame.fp = fault->r7;
	backtrace_frame.lr = fault->LR;
	backtrace_frame.sp = fault->SP;

	/* I'm not convinced this is correct,  */
	backtrace_frame.pc = fault->exception_return == 0xfffffff1 ? fault->LR : fault->PC;

	/* Try the unwind */
	int count = _backtrace_unwind(backtrace, array_sizeof(fault_backtrace), &backtrace_frame);

	/* Dump the trace */
	dump_backtrace(backtrace, count);
}

__noreturn void abort(void)
{
	struct backtrace abort_backtrace[25];

	int count = backtrace_unwind(abort_backtrace, array_sizeof(abort_backtrace));

	dump_backtrace(abort_backtrace, count);

	_exit(EXIT_FAILURE);
}

__noreturn void __assert_fail(const char *expr)
{
	if (expr)
		diag_printf("assert failed: %s\n", expr);

	__builtin_trap();
	abort();
}

static int console_put(char c, FILE *file)
{
	int fd = (int)file_get_platform(file);
	return write(fd, &c, 1);
}

static int console_get(FILE *file)
{
	char c;

	int fd = (int)file_get_platform(file);;
	int status = read(fd, &c, 1);
	return status < 0 ? status : c;
}

static void rtos_init(void)
{
	osStatus_t os_status = osKernelInitialize();
	if (os_status != osOK)
		syslog_fatal("failed to initialize the kernel: %d\n", os_status);
}
PREINIT_PLATFORM_WITH_PRIORITY(rtos_init, 500);

static __constructor_priority(550) void console_init(void)
{
	/* Open UART0 */
	int fd = open("ACM0", O_RDWR);
	if (fd < 0)
		syslog_fatal("could not open UART0: %d\n", errno);

	/* Initialize stdio to the console */
	old_stdio_iob = _stdio;
	old_stderr_iob = _stderr;
	iob_setup_stream(&_stdio, console_put, console_get, 0, 0, __SWR | __SRD |  __SPLATFORM , (void *)fd);
	iob_setup_stream(&_stderr, console_put, 0, 0, 0, __SWR | __SPLATFORM, (void *)fd);
}

static __destructor_priority(550) void console_destroy(void)
{
	struct iob stdio = _stdio;

	/* Restore the original stdio and stderr */
	_stdio = old_stdio_iob;
	_stderr = old_stderr_iob;

	/* Close the uart */
	close((int)iob_get_platform(&stdio));
}

int main(int argc, char **argv)
{
	osThreadSuspend(osThreadGetId());

	return EXIT_SUCCESS;
}

static void main_task(void *context)
{
	struct main_context *main_context = context;

	/* Common init function */
	_init();

	/* Execute the ini array */
	run_init_array(__init_array_start, __init_array_end);

	/* Run the main function */
	main_context->status = main(main_context->argc, main_context->argv);

	/* Execute the fini array */
	run_init_array(__fini_array_start, __fini_array_end);

	/* Common exit */
	_fini();
}

int _main(int argc, char **argv)
{
	struct main_context main_context = { .argc = argc, .argv = argv, .status = EXIT_SUCCESS };
	osThreadId_t main_task_id;

	/* Create the main task */
	osThreadAttr_t thread_attr = { .name = "main-task", .stack_size = 4096, .priority = osPriorityNormal };
	main_task_id = osThreadNew(main_task, &main_context, &thread_attr);
	if (!main_task_id)
		syslog_fatal("failed to create the main task: %d\n", errno);

	/* Start the kernel */
	osStatus_t os_status = osKernelStart();
	if (os_status != osOK)
		syslog_fatal("failed to start the kernel: %d\n", os_status);

	/* All done */
	return main_context.status;
}

