#include <errno.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include <sys/lock.h>
#include <sys/systick.h>
#include <sys/irq.h>
#include <sys/swi.h>
#include <sys/spinlock.h>
#include <sys/async.h>

#include <rtos/rtos-toolkit/scheduler.h>

#define LIBC_LOCK_MARKER 0x89988998
#define MULTICORE

struct __lock
{
	long value;
	struct futex futex;
	long count;
	unsigned int marker;
};

extern void _set_tls(void *tls);
extern void _init_tls(void *__tls_block);

extern __weak void osTimerTick(void);

void scheduler_switch_hook(struct task *task);
void scheduler_tls_init_hook(void *tls);
void scheduler_run_hook(bool start);
void scheduler_spin_lock(void);
void scheduler_spin_unlock(void);
unsigned int scheduler_spin_lock_irqsave(void);
void scheduler_spin_unlock_irqrestore(unsigned int state);

void _init(void);
void _fini(void);
void __libc_fini_array(void);

spinlock_t scheduler_spinlock = 0;

struct __lock __lock___libc_recursive_mutex = { 0 };

#if 0
/* Not C because of some bull shit undocumented gcc nonsense where r0-r3 are not saved by the caller as expected
 * combined with a long standing bug in which strong assembly symbols are not honored */
__naked void *__aeabi_read_tp(void)
{
	asm volatile (
		"ldr r0, =scheduler \n" /* Load the address of the schedule pointer */
		"ldr r0, [r0] \n"       /* Load pointer to the scheduler */
		"ldr r0, [r0] \n"       /* Load pointer to the current task (i.e. the first field) */
		"ldr r0, [r0, #4] \n"   /* Load the tls pointer (i.e. the second field of the current task */
		"mov pc, lr \n"
	);
}
#endif

/* We manage fini array invocation, ignore picolibc */
void __libc_fini_array(void)
{
}

void __retarget_lock_init(_LOCK_T *lock)
{
	assert(lock != 0);

	/* Get the space for the lock */
	if (*lock == 0) {
		*lock = sbrk(sizeof(struct __lock));
		if (!*lock)
			abort();
	}

	/* Initialize it */
	(*lock)->value = 0;
	(*lock)->count = -1;
	(*lock)->marker = LIBC_LOCK_MARKER;
	scheduler_futex_init(&(*lock)->futex, &(*lock)->value, SCHEDULER_FUTEX_PI | SCHEDULER_FUTEX_OWNER_TRACKING | SCHEDULER_FUTEX_CONTENTION_TRACKING);
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
	__retarget_lock_init(lock);
	(*lock)->count = 0;
}

void __retarget_lock_close(_LOCK_T lock)
{
	lock->marker = 0;
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
	__retarget_lock_close(lock);
}

void __retarget_lock_acquire(_LOCK_T lock)
{
	assert(lock != 0 && (lock->marker == LIBC_LOCK_MARKER || lock->marker == 0));

	/* Skip if closed */
	if (lock->marker == 0)
		return;

	/* Run the lock algo */
	long value = (long)scheduler_task();
	long expected = 0;
	while (!atomic_compare_exchange_strong(&lock->value, &expected, value)) {
		int status = scheduler_futex_wait(&lock->futex, expected, SCHEDULER_WAIT_FOREVER);
		if (status < 0)
			abort();

		/* Did we end up with the mutex due to contention tracking? */
		if (value == (lock->value & ~SCHEDULER_FUTEX_CONTENTION_TRACKING))
			break;

		/* Nope, try again */
		expected = 0;
	}
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
	assert(lock != 0 && (lock->marker == LIBC_LOCK_MARKER || lock->marker == 0));

	/* Skip if closed */
	if (lock->marker == 0)
		return;

	/* Handle recursive locks */
	long value = (long)scheduler_task();
	if (value == (lock->value & ~SCHEDULER_FUTEX_CONTENTION_TRACKING)) {
		++lock->count;
		return;
	}

	/* Forward */
	__retarget_lock_acquire(lock);

	/* Initialize the count for recursive locks */
	lock->count = 1;
}

int __retarget_lock_try_acquire(_LOCK_T lock)
{
	assert(lock != 0 && (lock->marker == LIBC_LOCK_MARKER || lock->marker == 0));

	/* Skip if closed */
	if (lock->marker == 0)
		return true;

	/* Just try update the lock bit */
	long value = (long)scheduler_task();
	long expected = 0;
	if (!atomic_compare_exchange_strong(&lock->value, &expected, value)) {
		errno = EBUSY;
		return false;
	}

	/* Got the lock */
	return true;
}void swi_trigger(unsigned int swi);


int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
	assert(lock != 0 && (lock->marker == LIBC_LOCK_MARKER || lock->marker == 0));

	/* Skip if closed */
	if (lock->marker == 0)
		return true;

	/* Handle recursive locks */
	long value = (long)scheduler_task();
	if (value == (lock->value & ~SCHEDULER_FUTEX_CONTENTION_TRACKING)) {
		++lock->count;
		return true;
	}

	/* Forward */
	int status = __retarget_lock_try_acquire(lock);
	if (!status)
		return false;

	/* Initialize the count for recursive locks, we have joy */
	lock->count = 1;
	return true;
}

void __retarget_lock_release(_LOCK_T lock)
{
	assert(lock != 0 && (lock->marker == LIBC_LOCK_MARKER || lock->marker == 0));

	/* Skip if closed */
	if (lock->marker == 0)
		return;

	/* Hot path unlock in the non-contented case */
	long expected = (long)scheduler_task();
	if (lock->value == expected && atomic_compare_exchange_strong(&lock->value, &expected, 0))
		return;

	/* Must have been contended */
	int status = scheduler_futex_wake(&lock->futex, false);
	if (status < 0)
		abort();
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
	assert(lock != 0 && (lock->marker == LIBC_LOCK_MARKER || lock->marker == 0));

	/* Skip if closed */
	if (lock->marker == 0)
		return;

	/* Handle recursive lock */
	if (--lock->count > 0)
		return;

	/* Forward */
	__retarget_lock_release(lock);
}

void scheduler_tls_init_hook(void *tls)
{
	_init_tls(tls);
}

void scheduler_switch_hook(struct task *task)
{
	void *tls = task != 0 ? task->tls : 0;
	_set_tls(tls);
}

static void systick_handler(void *context)
{
	/* Forward the to the core tick */
	scheduler_tick();

	/* Send to the cmsis timer tick */
	if (SystemCurrentCore == 0)
		osTimerTick();
}

void scheduler_run_hook(bool start)
{
	if (start) {

		/* Initialize the scheduler exception priorities */
		irq_set_priority(PendSV_IRQn, SCHEDULER_PENDSV_PRIORITY);
		irq_set_priority(SVCall_IRQn, SCHEDULER_SVC_PRIORITY);

		/* Initialize the core systick, no harm if already initialized */
		systick_init();

		/* Register a handler */
		systick_register_handler(systick_handler, 0);

	} else
		/* Just unregister the handler, it is ok if the systick is still running */
		systick_unregister_handler(systick_handler);
}

#ifdef MULTICORE

void scheduler_spin_lock()
{
	spin_lock(&scheduler_spinlock);
}

void scheduler_spin_unlock(void)
{
	spin_unlock(&scheduler_spinlock);
}

unsigned int scheduler_spin_lock_irqsave(void)
{
	return spin_lock_irqsave(&scheduler_spinlock);
}

void scheduler_spin_unlock_irqrestore(unsigned int state)
{
	spin_unlock_irqrestore(&scheduler_spinlock, state);
}

unsigned long scheduler_num_cores(void)
{
	return SystemNumCores;
}

unsigned long scheduler_current_core(void)
{
	return SystemCurrentCore;
}

extern void multicore_post(uintptr_t event);

void scheduler_request_switch(unsigned long core)
{
	/* Current core? */
	if (core == scheduler_current_core()) {
		irq_trigger(PendSV_IRQn);
		return;
	}

	/* System interrupt must be sent directly to the other core */
//	multicore_post(0x90000000 | (PendSV_IRQn + 16));
}

//static void multicore_trap(void)
//{
//	abort();
//}

void init_fault(void);
void init_fault(void)
{
//	multicore_post((uintptr_t)multicore_trap);
}

static void mulitcore_scheduler_run(struct async *async)
{
	/* start the scheduler running */
	scheduler_run();
}
static struct async multicore_scheduler_async;

#endif

void _init(void)
{
	_LOCK_T lock = &__lock___libc_recursive_mutex;
	__retarget_lock_init_recursive(&lock);

#ifdef MULTICORE
	async_run(&multicore_scheduler_async, mulitcore_scheduler_run, 0);
#endif
}

void _fini(void)
{
#ifdef MULTICORE
	async_cancel(&multicore_scheduler_async);
#endif

	_LOCK_T lock = &__lock___libc_recursive_mutex;
	__retarget_lock_close_recursive(lock);
}
