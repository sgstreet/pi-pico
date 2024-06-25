#ifndef _CMSIS_SYSTEM_RP2040_H
#define _CMSIS_SYSTEM_RP2040_H

#ifdef __cplusplus
extern "C" {
#endif

#include <compiler.h>

#define RP2040_CLOCK_GPIO0 0UL
#define RP2040_CLOCK_GPIO1 1UL
#define RP2040_CLOCK_GPIO2 2UL
#define RP2040_CLOCK_GPIO3 3UL
#define RP2040_CLOCK_REF 4UL
#define RP2040_CLOCK_SYS 5UL
#define RP2040_CLOCK_PERI 6UL
#define RP2040_CLOCK_USB 7UL
#define RP2040_CLOCK_ADC 8UL
#define RP2040_CLOCK_RTC 9UL
#define RP2040_NUM_CLOCKS (RP2040_CLOCK_RTC + 1)

#define SystemNumCores 2UL
#define SystemCurrentCore (*(volatile uint32_t *const)(0xd0000000))

extern uint32_t SystemCoreClock;

extern uint32_t rp2040_clocks[RP2040_NUM_CLOCKS];

extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);

extern void SystemResetCore(uint32_t core);
extern void SystemLaunchCore(uint32_t core, uintptr_t vector_table, uintptr_t stack_pointer, void (*entry_point)(void));
extern __noreturn void SystemExitCore(void);

#ifdef __cplusplus
}
#endif

#endif /* _CMSIS_SYSTEM_RP2040_H */
