#include <errno.h>
#include <alloca.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>
#include <sys/lock.h>

#include <rtos/rtos-toolkit/rtos-toolkit.h>

#define LIBC_LOCK_MARKER 0x89988998

struct __lock
{
	long value;
	struct futex futex;
	long count;
	unsigned int marker;
};

extern void _set_tls(void *tls);
extern void _init_tls(void *__tls_block);

void scheduler_switch_hook(struct task *task);
void scheduler_tls_init_hook(void *tls);

struct __lock __lock___libc_recursive_mutex;

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
	scheduler_futex_init(&(*lock)->futex, &(*lock)->value, SCHEDULER_FUTEX_PI | SCHEDULER_FUTEX_OWNER_TRACKING | SCHEDULER_FUTEX_CONTENTION_TRACKING);
	(*lock)->marker = LIBC_LOCK_MARKER;
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
	__retarget_lock_init(lock);
	(*lock)->count = 0;
}

void __retarget_lock_close(_LOCK_T lock)
{
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
}

void __retarget_lock_acquire(_LOCK_T lock)
{
	assert(lock != 0 && lock->marker == LIBC_LOCK_MARKER);

	/* Run the lock algo */
	long expected = 0;
	if (!atomic_compare_exchange_strong(&lock->value, &expected, (long)scheduler_task())) {
		int status = scheduler_futex_wait(&lock->futex, expected, 0);
		if (status < 0)
			abort();
		assert((long)scheduler_task() == (lock->value & ~SCHEDULER_FUTEX_CONTENTION_TRACKING));
	}
}

void __retarget_lock_acquire_recursive(_LOCK_T lock)
{
	assert(lock != 0 && lock->marker == LIBC_LOCK_MARKER && lock->count >= 0);

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
	assert(lock != 0 && lock->marker == LIBC_LOCK_MARKER);

	/* Just try update the lock bit */
	long value = (long)scheduler_task();
	long expected = 0;
	if (!atomic_compare_exchange_strong(&lock->value, &expected, value)) {
		errno = EBUSY;
		return false;
	}

	/* Got the lock */
	return true;
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock)
{
	assert(lock != 0 && lock->marker == LIBC_LOCK_MARKER && lock->count >= 0);

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
	assert(lock != 0 && lock->marker == LIBC_LOCK_MARKER);

	/* Hot path unlock in the non-contented case */
	long value = (long)scheduler_task();
	long expected = value;
	if (lock->value == expected && atomic_compare_exchange_strong(&lock->value, &expected, 0))
		return;

	/* Must have been contended */
	int status = scheduler_futex_wake(&lock->futex, false);
	if (status < 0)
		abort();
}

void __retarget_lock_release_recursive(_LOCK_T lock)
{
	assert(lock != 0 && lock->marker == LIBC_LOCK_MARKER && lock->count >= 0);

	/* Handle recursive lock */
	if (--lock->count > 0)
		return;

	/* Forward */
	__retarget_lock_release(lock);
}

void _init(void);
void _init(void)
{
	_LOCK_T lock = &__lock___libc_recursive_mutex;
	__retarget_lock_init_recursive(&lock);
}

void scheduler_tls_init_hook(void *tls)
{
	_init_tls(tls);
}

void scheduler_switch_hook(struct task *task)
{
	_set_tls(task->tls);
}
