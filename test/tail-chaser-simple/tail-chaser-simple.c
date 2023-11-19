#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alloca.h>

#include <sys/syslog.h>

#include <rtos/rtos.h>

#define NUM_TAILS 4

void *_rtos2_alloc(size_t size);
void _rtos2_release(void *ptr);

static unsigned int tails_pulled[NUM_TAILS] = { 0 };
static osThreadId_t chasers[NUM_TAILS] = { 0 };
static osEventFlagsId_t events;

void *_rtos2_alloc(size_t size)
{
	return calloc(1, size);
}

void _rtos2_release(void *ptr)
{
	free(ptr);
}

static void tail_chaser(void *context)
{
	unsigned int id = (unsigned int)context;
	unsigned int tail = (id + 1) % NUM_TAILS;

	while (osThreadFlagsGet() == 0) {

		/* Wait for our tailed to get pulled */
		uint32_t flags = osEventFlagsWait(events, 1UL << id, osFlagsWaitAll, osWaitForever);
		if (flags & osFlagsError)
			syslog_fatal("chaser %u failed to wait for tail yank: %d\n", id, (osStatus_t)flags);

		/* Yank of the next tail */
		flags = osEventFlagsSet(events, 1UL << tail);
		if (flags & osFlagsError)
			syslog_fatal("chaser %u failed to yank tail: %d\n", id, (osStatus_t)flags);

		/* update the number of tails pulled */
		++tails_pulled[id];
	}
}

int main(int argc, char **argv)
{
	syslog_info("creating shared event\n");
	osEventFlagsAttr_t event_attr = { .name = "chaser-events" };
	events = osEventFlagsNew(&event_attr);
	if (!events)
		syslog_fatal("failed to create shared events: %d\n", -errno);

	syslog_info("creating %u tail chasers\n", NUM_TAILS);
	for (size_t i = 0; i < NUM_TAILS; ++ i) {
		char name[RTOS_NAME_SIZE];
		sprintf(name, "chaser-%u", i);
		syslog_info("starting %s\n", name);
		osThreadAttr_t thread_attr = { .name = name, .attr_bits = osThreadJoinable };
		chasers[i] = osThreadNew(tail_chaser, (void *)i, &thread_attr);
		if (!chasers[i])
			syslog_fatal("failed to create chaser %u: %d\n", i, -errno);
	}

	/* Start the chase by pulling the tail of the first chaser */
	syslog_info("pulling the first tail\n");
	uint32_t flags = osEventFlagsSet(events, 0x00000001);
	if (flags != 0x00000001)
		syslog_fatal("failed to yank tail 0: %d\n", (osStatus_t)flags);

	/* Change priority */
	syslog_info("raising the main thread priority\n");
	osStatus_t os_status = osThreadSetPriority(osThreadGetId(), osPriorityAboveNormal);
	if (os_status != osOK)
		syslog_fatal("failed to raise priority of main thread: %d\n", os_status);

	/* Dump counts every second */
	syslog_info("monitoring the chasers results\n");
	for (int cycles = 0; cycles < 30; ++cycles) {

		/* Wait awhile */
		osStatus_t os_status = osDelay(1000);
		if (os_status != osOK)
			syslog_fatal("failed to delay: %d", os_status);

		/* Dump the state */
		char msg[64];
		char *addr = msg;
		addr += sprintf(addr, "tail pulled: ");
		for (size_t i = 0; i < NUM_TAILS; ++i)
			addr += sprintf(addr, "%u ", tails_pulled[i]);
		*addr = 0;
		syslog_info("%s\n", msg);
	}

	/* Kill all the threads */
	for (size_t i = 0; i < NUM_TAILS; ++i) {

		/* Tell the thread to exit */
		syslog_info("terminating %s\n", osThreadGetName(chasers[i]));
		uint32_t flags = osThreadFlagsSet(chasers[i], 0x00000001);
		if (flags & osFlagsError)
			syslog_fatal("failed ot set thread termination flag: %d\n", (osStatus_t)flags);

		/* Make sure the thread is awake */
		flags = osEventFlagsSet(events, 1UL << i);
		if (flags & osFlagsError)
			syslog_fatal("failed ot set thread termination flag: %d\n", (osStatus_t)flags);

		/* Wait for the thread to exit */
		osStatus_t os_status = osThreadJoin(chasers[i]);
		if (os_status != osOK)
			syslog_fatal("failed to join with %s %p: %d\n", osThreadGetName(chasers[i]), chasers[i], os_status);
	}

	return 0;
}
