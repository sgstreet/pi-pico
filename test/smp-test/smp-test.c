#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/syslog.h>
#include <sys/systick.h>
#include <sys/backtrace.h>
#include <sys/fault.h>
#include <sys/swi.h>
#include <sys/irq.h>

#include <rtos/rtos.h>

#define NUM_HOGS 8

struct hog
{
	osThreadId_t id;
	unsigned int loops;
	unsigned int cores[2];
};

struct hog hogs[NUM_HOGS] = { 0 };
unsigned int swi_handler_counter = 0;
unsigned int swi_kick_counter = 0;
unsigned int swi_cores[2] = { 0, 0};
osThreadId_t swi_counter_id = 0;
osEventFlagsId_t events;

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

__noreturn void reset_fault(const struct cortexm_fault *fault)
{
	__BKPT(10);
	abort();
}

static void swi_handler(unsigned int swi, void *context)
{
//	uint32_t flags = osThreadFlagsSet(swi_counter_id, 1);
//	assert(flags == 1);
	__unused uint32_t flags = osEventFlagsSet(events, 1);
	assert(flags == 1);
	++swi_handler_counter;
}

static void swi_counter_task(void *context)
{
	while (true) {

//		uint32_t event = osThreadFlagsWait(1, osFlagsWaitAll, osWaitForever);
		__unused uint32_t event = osEventFlagsWait(events, 1, osFlagsWaitAll, osWaitForever);
		assert((event & osFlagsError) == 0);
		++swi_cores[SystemCurrentCore()];
	}
}

static void hog_task(void *context)
{
	struct hog *hog = context;

	while (true) {
		unsigned int core = SystemCurrentCore();
		++hog->loops;
		++hog->cores[core];
		if ((random() & 0x8) == 0) {
			swi_trigger(5);
			++swi_kick_counter;
			osThreadYield();
		}
	}
}

int main(int argc, char **argv)
{
	irq_set_core(SWI_5_IRQn, 1);
	swi_register(5, INTERRUPT_NORMAL, swi_handler, 0);
	swi_enable(5);

	osEventFlagsAttr_t events_attr = { .name = "swi-events" };
	events = osEventFlagsNew(&events_attr);
	if (!events)
		syslog_fatal("failed to create swi event flags\n");

	osThreadAttr_t swi_counter_thread_attr = { .name = "swi-counter", .priority = osPriorityHigh };
	swi_counter_id = osThreadNew(swi_counter_task, 0, &swi_counter_thread_attr);
	if (!swi_counter_id)
		syslog_fatal("failed to start swi_counter_task\n");

	syslog_info("launching %u hogs\n", NUM_HOGS);
	for (int i = 0; i < NUM_HOGS; ++i) {
		osThreadAttr_t hog_thread_attr = { .name = "hog", .priority = osPriorityNormal };
		hogs[i].id = osThreadNew(hog_task, &hogs[i], &hog_thread_attr);
		if (!hogs[i].id)
			syslog_fatal("failed to start hog %d\n", i);
	}

	/* Dump hog info */
	while (true) {
		printf("---\n");
		printf("\tswi = [%u, %u, %u, %u]\n", swi_kick_counter, swi_handler_counter, swi_cores[0], swi_cores[1]);
		for (int i = 0; i < NUM_HOGS; ++i)
			printf("\thog[%d] = [%u, %u]\n", i, hogs[i].cores[0], hogs[i].cores[1]);
		osDelay(1000);
	}
}
