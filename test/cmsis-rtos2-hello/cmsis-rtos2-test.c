#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/fault.h>
#include <sys/syslog.h>
#include <sys/iob.h>

#include <rtos/rtos.h>

void save_fault(const struct cortexm_fault *fault, const struct backtrace *entries, int count)
{
	for (size_t i = 0; i < count; ++i)
		syslog_error("%s@%p - %p\n", entries[i].name, entries[i].function, entries[i].address);
}

__noreturn void __assert_fail(const char *expr)
{
	syslog_fatal("assert failed: %s\n", expr);
}

int main(int argc, char **argv)
{
	printf("cmsis rto2 hello!\n");
	osDelay(10);
	return EXIT_SUCCESS;
}
