#include <compiler.h>
#include <stdlib.h>

#include <sys/backtrace.h>
#include <diag/diag.h>

#include <sys/syslog.h>

const char *syslog_level_str[] = { "NONE", "FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE" };

__weak __noreturn void syslog_fatal_abort(const char *fmt, ...)
{
	int count = SYSLOG_BACKTRACE_SIZE;
	backtrace_t backtrace[SYSLOG_BACKTRACE_SIZE];

	/* Capture the backtrace */
	count = backtrace_unwind(backtrace, count);

	/* Setup the arguments */
	va_list args;
	va_start(args, fmt);

	/* Dump the backtrace */
	diag_vprintf(fmt, args);
	diag_printf("FATAL %s - aborting with backtrace:\n", backtrace_function_name((uint32_t)__builtin_return_address(0)));
	for (int i = 0; i < count; ++i)
		diag_printf("%p@%p - %s\n", backtrace[i].function, backtrace[i].address, backtrace[i].name);

	/* All done */
	va_end(args);

	/* Should not return */
	abort();
}

__weak void syslog(int level, const char *fmt, ...)
{
	/* Setup the arguments */
	va_list args;
	va_start(args, fmt);

	if (level != SYSLOG_NONE)
		fprintf(stddiag, "%s %s - ", syslog_level_str[level], backtrace_function_name((uint32_t)__builtin_return_address(0)));

	vfprintf(stddiag, fmt, args);

	/* All done */
	va_end(args);
}
