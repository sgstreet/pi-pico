#include <asm.h>

declare_function crc32_small, .text
	.fnstart

	push {r4, r5, lr}

	// r1 now end
	add r1, r0
	ldr r5, =#0x4c11db7

	b byte_loop_test

byte_loop:
    ldrb r4, [r0]

    rev r3, r2
    eors r4, r3
    lsls r4, #24

    movs r3, #8

bit_loop:
    lsls r4, #1
    bcc no_bit
    eors r4, r5

no_bit:
    subs r3, #1
    bne bit_loop

    lsls r2, #8
    eors r2, r4

    adds r0, #1

byte_loop_test:
    cmp r0, r1
    blt byte_loop

    mov r0, r2
    pop {r4, r5, pc}

   	.fnend
	.pool
	.size	crc32_small, . - crc32_small
