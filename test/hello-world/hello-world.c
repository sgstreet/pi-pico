#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <board/board.h>

#include <sys/systick.h>

int main(int argc, char **argv)
{
	while (true) {
		printf("hello world!: %lu\n", systick_get_ticks());
		board_led_toggle(0);
		systick_delay(250);
	}
}

