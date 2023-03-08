#include <asm.h>

declare_function delay_cycles, .fast
	.fnstart

.L0:
	subs	r0, r0, #1
	bne		.L0
	mov		pc, lr

	.fnend
	.pool
	.size	delay_cycles, . - delay_cycles
