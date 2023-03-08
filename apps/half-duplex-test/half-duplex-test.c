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

static bool hd0_state = false;
static bool hd1_state = false;
static int hd0_count = 0;
static int hd1_count = 0;

static char *strrev(char *str)
{
	int i;
	int j;
	unsigned char a;

	if (!str || !*str)
		return str;

	unsigned len = strlen(str);
	for (i = 0, j = len - 1; i < j; i++, j--)
	{
		a = str[i];
		str[i] = str[j];
		str[j] = a;
	}

	return str;
}

static __unused void hd0_task(void *context)
{
	assert(context != 0);

	char buffer[256] = "The quick brown fox jumps over the lazy dog";
	struct half_duplex *hd = context;

	osThreadSuspend(osThreadGetId());

	while (true) {
		++hd0_count;

		hd0_state = false;
		half_duplex_send(hd, buffer, strlen(buffer));

		hd0_state = true;
		uint32_t amount = half_duplex_recv(hd, buffer, 256, osWaitForever);

		osThreadYield();

		buffer[amount] = 0;
		strrev(buffer);
	}
}

static __unused void hd1_task(void *context)
{
	assert(context != 0);

	char buffer[256];
	struct half_duplex *hd = context;

	size_t amount = strlen(msg);

	while (true) {
		++hd1_count;

		hd1_state = true;
		amount = half_duplex_recv(hd, buffer, 256, osWaitForever);

		osThreadYield();

		buffer[amount] = 0;
		strrev(buffer);

		hd1_state = false;
		half_duplex_send(hd, buffer, strlen(buffer));

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

	osThreadAttr_t hd0_task_attr = { .name = "hd0", .stack_size = 2048, .priority = osPriorityAboveNormal };
	osThreadId_t hd0_task_id = osThreadNew(hd0_task, &hd0, &hd0_task_attr);
	if (!hd0_task_id)
		abort();

	struct half_duplex hd1;
	half_duplex_ini(&hd1, 1, 12, 1);

	osThreadAttr_t hd1_task_attr = { .name = "hd1", .stack_size = 2048, .priority = osPriorityAboveNormal };
	osThreadId_t hd1_task_id = osThreadNew(hd1_task, &hd1, &hd1_task_attr);
	if (!hd1_task_id)
		abort();

	osThreadResume(hd0_task_id);

	/* Let the ping pong threads run forever1 */
	osThreadSuspend(osThreadGetId());
}

