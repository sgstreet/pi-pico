#include <asm.h>

declare_function Reset_Handler, .isr
	.fnstart
	.cantunwind

	cpsid    i
	ldr      r0, =0xe000ed08
	ldr      r1, =__vector
	str      r1, [r0]
	ldr      r2, [r1, #0]
	mov      sp, r2

	bl       SystemInit

    ldr      r4, =__copy_table_start__
    ldr      r5, =__copy_table_end__

.L_loop0:
    cmp      r4, r5
    bge      .L_loop0_done
    ldr      r1, [r4]                /* source address */
    ldr      r2, [r4, #4]            /* destination address */
    ldr      r3, [r4, #8]            /* byte count */
    beq     .L_loop0_0_done          /* Skip if nothing to be copied */

.L_loop0_0:
    subs     r3, #4                  /* decrement byte count */
    blt      .L_loop0_0_done
    ldr      r0, [r1, r3]
    str      r0, [r2, r3]
    b        .L_loop0_0

.L_loop0_0_done:
    adds     r4, #12
    b        .L_loop0

.L_loop0_done:
    ldr      r3, =__zero_table_start__
    ldr      r4, =__zero_table_end__

.L_loop2:
    cmp      r3, r4
    bge      .L_loop2_done
    ldr      r1, [r3]                /* destination address */
    ldr      r2, [r3, #4]            /* btye count */
    movs     r0, 0

.L_loop2_0:
    subs     r2, #4                  /* decrement byte count */
    blt      .L_loop2_0_done
    str      r0, [r1, r2]
    b        .L_loop2_0
.L_loop2_0_done:
    adds     r3, #8
    b        .L_loop2

.L_loop2_done:
    bl       _start

	.fnend
	.pool
	.size	Reset_Handler, . - Reset_Handler
