#include <asm.h>

	.section .vector, "a", %progbits
	.align 2
	.globl __isr_vector
__isr_vector:
	.word __stack
	.word Reset_Handler
	.word NMI_Handler
	.word HardFault_Handler
	.word MemManage_Handler
	.word BusFault_Handler
	.word UsageFault_Handler
	.word 0
	.word 0
	.word 0
	.word 0
	.word SVC_Handler
	.word DebugMon_Handler
	.word 0
	.word PendSV_Handler
	.word SysTick_Handler
	.word TIMER_IRQ_0_Handler
	.word TIMER_IRQ_1_Handler
	.word TIMER_IRQ_2_Handler
	.word TIMER_IRQ_3_Handler
	.word PWM_IRQ_WRAP_Handler
	.word USBCTRL_IRQ_Handler
	.word XIP_IRQ_Handler
	.word PIO0_IRQ_0_Handler
	.word PIO0_IRQ_1_Handler
	.word PIO1_IRQ_0_Handler
	.word PIO1_IRQ_1_Handler
	.word DMA_IRQ_0_Handler
	.word DMA_IRQ_1_Handler
	.word IO_IRQ_BANK0_Handler
	.word IO_IRQ_QSPI_Handler
	.word SIO_IRQ_PROC0_Handler
	.word SIO_IRQ_PROC1_Handler
	.word CLOCKS_IRQ_Handler
	.word SPI0_IRQ_Handler
	.word SPI1_IRQ_Handler
	.word UART0_IRQ_Handler
	.word UART1_IRQ_Handler
	.word ADC_IRQ_FIFO_Handler
	.word I2C0_IRQ_Handler
	.word I2C1_IRQ_Handler
	.word RTC_IRQ_Handler
	.word SWI_0_IRQ_Handler
	.word SWI_1_IRQ_Handler
	.word SWI_2_IRQ_Handler
	.word SWI_3_IRQ_Handler
	.word SWI_4_IRQ_Handler
	.word SWI_5_IRQ_Handler

	.size __isr_vector, . - __isr_vector

declare_weak_function default_handler, .isr
	.fnstart
/*	.cantunwind */

	/* Load the active exception */
	mrs r0, ipsr

	/* Load the address of the irq_table[irq] */
	ldr r2, =irq_dispatch
	lsls r1, r0, #3
	add r2, r2, r1

	/* Adjust to a IRQn_Type */
	subs r0, r0, #16

	/* load the irq context */
	ldr r1, [r2, #4]

	/* Jump to the handler address. The handler will perform the isr return */
	ldr r2, [r2, #0]
	mov pc, r2

	.fnend
	.pool
	.size default_handler, . - default_handler

.macro def_irq_handler handler_name
	.weak \handler_name
	.thumb_set \handler_name, default_handler
.endm

	/* Bind the weak default handlers */
	def_irq_handler Reset_Handler
	def_irq_handler NMI_Handler
	def_irq_handler HardFault_Handler
	def_irq_handler MemManage_Handler
	def_irq_handler BusFault_Handler
	def_irq_handler UsageFault_Handler
	def_irq_handler SVC_Handler
	def_irq_handler DebugMon_Handler
	def_irq_handler PendSV_Handler
	def_irq_handler SysTick_Handler
	def_irq_handler TIMER_IRQ_0_Handler
	def_irq_handler TIMER_IRQ_1_Handler
	def_irq_handler TIMER_IRQ_2_Handler
	def_irq_handler TIMER_IRQ_3_Handler
	def_irq_handler PWM_IRQ_WRAP_Handler
	def_irq_handler USBCTRL_IRQ_Handler
	def_irq_handler XIP_IRQ_Handler
	def_irq_handler PIO0_IRQ_0_Handler
	def_irq_handler PIO0_IRQ_1_Handler
	def_irq_handler PIO1_IRQ_0_Handler
	def_irq_handler PIO1_IRQ_1_Handler
	def_irq_handler DMA_IRQ_0_Handler
	def_irq_handler DMA_IRQ_1_Handler
	def_irq_handler IO_IRQ_BANK0_Handler
	def_irq_handler IO_IRQ_QSPI_Handler
	def_irq_handler SIO_IRQ_PROC0_Handler
	def_irq_handler SIO_IRQ_PROC1_Handler
	def_irq_handler CLOCKS_IRQ_Handler
	def_irq_handler SPI0_IRQ_Handler
	def_irq_handler SPI1_IRQ_Handler
	def_irq_handler UART0_IRQ_Handler
	def_irq_handler UART1_IRQ_Handler
	def_irq_handler ADC_IRQ_FIFO_Handler
	def_irq_handler I2C0_IRQ_Handler
	def_irq_handler I2C1_IRQ_Handler
	def_irq_handler RTC_IRQ_Handler
	def_irq_handler SWI_0_IRQ_Handler
	def_irq_handler SWI_1_IRQ_Handler
	def_irq_handler SWI_2_IRQ_Handler
	def_irq_handler SWI_3_IRQ_Handler
	def_irq_handler SWI_4_IRQ_Handler
	def_irq_handler SWI_5_IRQ_Handler
