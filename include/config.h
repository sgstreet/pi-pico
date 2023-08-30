#ifndef CONFIG_H_
#define CONFIG_H_

#include <board/board.h>
#include <stddef.h>

#define INTERRUPT_REALTIME 0
#define INTERRUPT_NORMAL 1
#define INTERRUPT_BELOW_NORMAL 2

#define SCHEDULER_PRIOR_BITS 0x00000002UL
#define SCHEDULER_MAX_IRQ_PRIORITY INTERRUPT_NORMAL
#define SCHEDULER_MIN_IRQ_PRIORITY (SCHEDULER_PRIOR_BITS + 1)

#define SYSTEM_INIT_PRIORITY 000
#define FAULT_SYSTEM_INIT_PRIORITY 010
#define IRQ_SYSTEM_INIT_PRIORIY 020
#define SWI_SYSTEM_INIT_PRIORIY 021
#define SYSTICK_SYSTEM_INIT_PRIORITY 030

#define BOARD_PLATFORM_INIT_PRIORITY 000
#define BOARD_PLATFORM_DIAG_PRIORITY 010

#define HAL_PLATFORM_PRIORITY 020

#define DEVICES_PLATFORM_PRIORITY 030

#define DIAG_BUFFER_SIZE 256

#define ATOMIC_HW_SPINLOCK 0UL
#define SCHEDULER_HW_SPINLOCK 1UL
#define ATOMIC_SWI_SPINLOCK 2UL

#define SYSTICK_INTERRUPT_PRIORITY INTERRUPT_NORMAL
#define SVCALL_INTERRUPT_PRIORITY (INTERRUPT_NORMAL + 1)
#define PENDSV_INTERRUPT_PRIORITY (INTERRUPT_NORMAL + 2)

#define MULTICORE_STACK_SIZE 4096UL

#define SCHEDULER_TIME_SLICE UINT32_MAX/*10UL*/

#define CONSOLE_BAUD_RATE 115200UL
#define CONSOLE_BUFFER_SIZE 32UL

#define SERVICES_PRIORITY 200

#define SHELL_COMMAND_POOL_SIZE 16
#define SHELL_COMMAND_STACK_SIZE 4096

#define LOGGER_MSG_SIZE 128UL
#define LOGGER_QUEUE_SIZE 16UL
#define LOGGER_STACK_SIZE 512UL
#define LOGGER_BAUD_RATE 115200
#define LOGGER_HALF_DUPLEX_CHANNEL 1UL

#define DYNAMIXEL_PORTS_BAUD_RATE 57600U
#define DYNAMIXEL_PORTS_RX_IDLE_TIMEOUT 10U
#define DYNAMIXEL_PORTS_HALF_DUPLEX_CHANNELS { 0 }

#define LEG_MANAGER_LEG_DYNAMIXEL_PORTS { 0 }

#endif
