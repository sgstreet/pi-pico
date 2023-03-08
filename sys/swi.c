#include <compiler.h>

#include <sys/irq.h>
#include <sys/swi.h>

struct swi_entry
{
	swi_handler_t handler;
	void *context;
};

static void swi_default_handler(unsigned int swi, void *context)
{
}

static struct swi_entry swi_dispatch[BOARD_SWI_NUM] =  { [0 ... BOARD_SWI_NUM - 1] = { .handler = swi_default_handler, .context = 0 } };

static __isr_section __optimize void dispatch_swi(IRQn_Type type, void *context)
{
	unsigned int swi = (unsigned int)context;
	swi_dispatch[swi].handler(swi, swi_dispatch[swi].context);
}

void swi_register(unsigned int swi, uint32_t priority, swi_handler_t handler, void *context)
{
	assert(swi < BOARD_SWI_NUM);

	IRQn_Type irq = board_swi_lookup(swi);

	swi_dispatch[swi].context = context;
	swi_dispatch[swi].handler = handler;

	irq_register(irq, priority, dispatch_swi, (void *)swi);
	irq_enable(irq);
}

void swi_unregister(unsigned int swi, swi_handler_t handler)
{
	assert(swi < BOARD_SWI_NUM);
	irq_unregister(board_swi_lookup(swi), dispatch_swi);
}

void swi_set_priority(unsigned int swi, unsigned int priority)
{
	assert(swi < BOARD_SWI_NUM);
	irq_set_priority(board_swi_lookup(swi), priority);
}

unsigned int swi_get_priority(unsigned int swi)
{
	assert(swi < BOARD_SWI_NUM);
	return irq_get_priority(board_swi_lookup(swi));
}

void swi_enable(unsigned int swi)
{
	assert(swi < BOARD_SWI_NUM);
	irq_enable(board_swi_lookup(swi));
}

void swi_disable(unsigned int swi)
{
	assert(swi < BOARD_SWI_NUM);
	irq_disable(board_swi_lookup(swi));
}

bool swi_is_enabled(unsigned int swi)
{
	assert(swi < BOARD_SWI_NUM);
	return irq_is_enabled(board_swi_lookup(swi));
}

void swi_trigger(unsigned int swi)
{
	assert(swi < BOARD_SWI_NUM);
	irq_trigger(board_swi_lookup(swi));
}
