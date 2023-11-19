#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <board/board.h>

#include <sys/systick.h>
#include <sys/async.h>

extern void multicore_post(void *addr);

static unsigned int callback_counter = 0;

static void blink_async(struct async *async)
{
	systick_init();

	while (!async_is_canceled(async)) {
		board_led_toggle(0);
		systick_delay(125);
	}
}

static void multicore_callback(void)
{
	++callback_counter;
}

int main(int argc, char **argv)
{
	static struct async async = { .context = 0, .func = 0, .done = true, .cancel = false };

	while (true) {
		printf("hello world!: %u\n", callback_counter);
		if ((callback_counter & 0x1f) == 0) {
			if (async_is_done(&async))
				async_run(&async, blink_async, 0);
			else
				async_cancel(&async);
		}
		multicore_post(multicore_callback);
		systick_delay(500);
	}
}

