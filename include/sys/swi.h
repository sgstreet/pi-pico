#ifndef _SWI_H_
#define _SWI_H_

#include <board/board.h>

typedef  void (*swi_handler_t)(unsigned int swi, void *context);

void swi_register(unsigned int swi, uint32_t priority, swi_handler_t handler, void *context);
void swi_unregister(unsigned int swi, swi_handler_t handler);

void swi_set_priority(unsigned int swi, unsigned int priority);
unsigned int swi_get_priority(unsigned int swi);

void swi_enable(unsigned int swi);
void swi_disable(unsigned int swi);
bool swi_is_enabled(unsigned int swi);

void swi_trigger(unsigned int swi);

#endif
