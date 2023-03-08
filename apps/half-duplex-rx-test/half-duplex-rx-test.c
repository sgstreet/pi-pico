#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <board/board.h>
#include <sys/systick.h>
#include <devices/half-duplex.h>

extern void half_duplex_tx_test(struct half_duplex *hd, const void *buffer, size_t count);
extern void half_duplex_rx_test(struct half_duplex *hd, void *buffer, size_t count);

/*                                  1         2         3         4
 *                         1234567890123456789012345678901234567890123 4 */
static const char msg[] = "The quick brown fox jumps over the lazy dog";

static unsigned int rx_cntr = 0;
static unsigned int rx_error_cntr = 0;

static void rx_task(void *context)
{
	assert(context != 0);

	char buffer[256];
	struct half_duplex *hd = context;

	while (true) {
		++rx_cntr;

		ssize_t amount = half_duplex_recv(hd, buffer, array_sizeof(buffer), osWaitForever);
		if (amount > 0) {
			buffer[amount] = 0;
			printf("%u: %u - %s\n", rx_cntr, amount, buffer);
		} else
			++rx_error_cntr;
	}
}

static __unused bool dump_schedule_nodes(struct sched_list *node, void *context)
{
	__unused struct task *task = container_of(node, struct task, scheduler_node);
	printf("%s - %d\n", task->name, task->state);
	return true;
}

// void scheduler_for_each(struct sched_list *list, for_each_sched_node_t func, void *context);
static bool dump = false;
extern struct scheduler *scheduler;
void scheduler_idle_hook(void);
void scheduler_idle_hook(void)
{
	__WFI();
	if (dump)
		scheduler_for_each(&scheduler->tasks, dump_schedule_nodes, 0);
}

int main(int argc, char **argv)
{
	PADS_BANK0->GPIO13 &= ~(PADS_BANK0_GPIO13_OD_Msk | PADS_BANK0_GPIO13_PDE_Msk);
	PADS_BANK0->GPIO13 |= (PADS_BANK0_GPIO13_IE_Msk | PADS_BANK0_GPIO0_PUE_Msk);
	IO_BANK0->GPIO13_CTRL = (6UL << IO_BANK0_GPIO13_CTRL_FUNCSEL_Pos);

	PADS_BANK0->GPIO12 &= ~(PADS_BANK0_GPIO12_OD_Msk | PADS_BANK0_GPIO12_PDE_Msk);
	PADS_BANK0->GPIO12 |= (PADS_BANK0_GPIO12_IE_Msk | PADS_BANK0_GPIO0_PUE_Msk);
	IO_BANK0->GPIO12_CTRL = (6UL << IO_BANK0_GPIO12_CTRL_FUNCSEL_Pos);

	struct half_duplex hd0;
	half_duplex_ini(&hd0, 0, 13, 0);

	struct half_duplex hd1;
	half_duplex_ini(&hd1, 1, 12, 1);

	osThreadAttr_t rx_task_attr = { .name = "rx", .stack_size = 2048, .priority = osPriorityAboveNormal };
	osThreadId_t rx_task_id = osThreadNew(rx_task, &hd1, &rx_task_attr);
	if (!rx_task_id)
		abort();

	osDelay(1000);

	while (true) {
		half_duplex_send(&hd0, msg, strlen(msg));
		osDelay(100);
	}

}

