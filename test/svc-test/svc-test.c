#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <board/board.h>

#include <sys/systick.h>
#include <sys/svc.h>

void SVC_Handler_1(unsigned int svc, struct svc_frame *svc_frame);
void SVC_Handler_1(unsigned int svc, struct svc_frame *svc_frame)
{
	static int counter = 0;

	if (svc_frame->r1)
		board_led_on(svc_frame->r0);
	else
		board_led_off(svc_frame->r0);

	svc_frame->r0 = ++counter;
}

int main(int argc, char **argv)
{
	while (true) {
		systick_delay(250);
		svc_call2(1, 0, 1);
		systick_delay(250);
		int value = svc_call2(1, 0, 0);
		printf("counter: %d\n", value);
	}
}

