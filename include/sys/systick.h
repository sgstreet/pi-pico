#ifndef SYSTICK_H_
#define SYSTICK_H_

#define SYSTICK_MAX_MANAGED_HANDLERS 4

#define __SYSTICK_PRIORITY(PRIORITY) ".systick_array.00" #PRIORITY
#define __SYSTICK_SECTION(SECTION) ".systick_array." #SECTION
#define DECLARE_SYSTICK(NAME) void *__attribute__((used, section(__SYSTICK_SECTION(NAME)))) systick_##NAME = NAME
#define DECLARE_SYSTICK_WITH_PRIORITY(NAME, PRIORITY) void *__attribute__((used, section(__SYSTICK_PRIORITY(PRIORITY)))) systick_##NAME = NAME

unsigned long systick_get_ticks(void);
void systick_delay(unsigned long msecs);

#endif
