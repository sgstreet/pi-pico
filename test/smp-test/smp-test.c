#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <sys/syslog.h>
#include <hal/hal.h>

#include <rtos/rtos.h>

#define NUM_HOGS 4

struct hog
{
	osThreadId_t id;
	unsigned int loops;
	unsigned int cores[2];
};

struct hog hogs[NUM_HOGS] = { 0 };

static void hog_task(void *context)
{
	struct hog *hog = context;

	while (true) {
		unsigned int core = SystemCurrentCore();
		++hog->loops;
		++hog->cores[core];
	}
}

int main(int argc, char **argv)
{
	syslog_info("launching %u hogs\n", NUM_HOGS);
	for (int i = 0; i < NUM_HOGS; ++i) {
		osThreadAttr_t hog_thread_attr = { .name = "hog", .priority = osPriorityNormal };
		hogs[i].id = osThreadNew(hog_task, &hogs[i], &hog_thread_attr);
		if (!hogs[i].id)
			syslog_fatal("failed to start hog %d\n", i);
	}

	/* We never exit */
	osThreadSuspend(osThreadGetId());
}
