#ifndef __THREADS_H_
#define __THREADS_H_

#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>

#include <linked-list.h>

#include <rtos/rtos-toolkit/rtos-toolkit.h>

/*
To make an efficient implementation, the std::once_flag could have three states:

    execution finished: if you find this state, you're already done.
    execution in progress: if you find this: wait until it changes to finished (or changes to failed-with-exception, which which case try to claim it)
    execution not started: if you find this, attempt to CAS it to in-progress and call the function. If the CAS failed, some other thread succeeded, so goto the wait-for-finished state.

Checking the flag with an acquire-load is extremely cheap on most architectures (especially x86 where all loads are acquire-loads). 
Once it's set to "finished", it's not modified for the rest of the program, so it can stay cached in L1 on all cores (unless you put 
it in the same cache line as something that is frequently modified, creating false sharing).
*/

#define __THRD_STACK_SIZE 2048
#define __THRD_SIZE (__THRD_STACK_SIZE)
#define __THRD_PRIORITY 23
#define __THRD_KEYS_MAX 8
#define __THRD_TSS_SIZE (sizeof(void *) * __THRD_KEYS_MAX)
#define __THRD_MARKER 0x137cc731UL

#define TSS_DTOR_ITERATIONS 5

#define ONCE_FLAG_INIT 0

typedef atomic_int once_flag;
typedef uintptr_t thrd_t;
typedef size_t tss_t;

enum {
	mtx_prio_inherit = 0x8
};

typedef struct mtx
{
	long value;
	struct futex futex;
	unsigned long type;
	long count;
} mtx_t;

typedef struct cnd
{
	struct mtx *mutex;
	unsigned long sequence;
	struct futex futex;
} cnd_t;

struct tss
{
	atomic_bool used;
	void (*destructor)(void *);
};

struct thrd
{
	int (*func)(void *);
	void *context;
	int ret;
	struct task *task;
	bool detached;
	bool terminated;
	thrd_t joiner;
	struct cnd joiners;
	struct linked_list thrd_node;
	unsigned long marker;
	char stack[] __attribute__((aligned(8)));
};

struct thrd_attr
{
	char name[TASK_NAME_LEN];
	unsigned long flags;
	unsigned long priority;
	size_t stack_size;
};

int _thrd_ini(void);
void _thrd_fini(void);

void _thdr_attr_init(struct thrd_attr *attr, const char *name, unsigned long flags, unsigned long priority, size_t stack_size);
int	_thrd_create(thrd_t *thrd, int (*func)(void *), void *arg, struct thrd_attr *attr);
int _thrd_sleep(unsigned long msec);

#endif
