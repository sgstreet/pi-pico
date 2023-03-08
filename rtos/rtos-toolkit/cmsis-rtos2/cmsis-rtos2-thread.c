#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <rtos/rtos-toolkit/rtos-toolkit.h>

#define RTOS_REAPER_EXIT 0x00000001
#define RTOS_REAPER_CLEAN 0x00000002
#define RTOS_THREAD_JOINED 0x40000000

struct rtos2_thread_capture
{
	uint32_t count;
	uint32_t size;
	osThreadId_t *threads;
};

extern void *_rtos2_alloc(size_t size);
extern void _rtos2_release(void *ptr);

extern __weak struct rtos_thread *_rtos2_alloc_thread(size_t stack_size);
extern __weak void _rtos2_release_thread(struct rtos_thread *thread);
extern __weak void _rtos2_thread_stack_overflow(struct rtos_thread *thread);

extern void *__tls_size;

const size_t osThreadMinimumStackSize = sizeof(struct task) + (size_t)&__tls_size + sizeof(struct scheduler_frame) + 8;

static thread_local struct rtos_thread *current_thread = 0;

static osThreadId_t reaper_thread = 0;
static osOnceFlag_t reaper_thread_init = osOnceFlagsInit;

__weak struct rtos_thread *_rtos2_alloc_thread(size_t stack_size)
{
	/* Calculate the required size needed to provide the request stack size and make it a multiple of 8 bytes */
	size_t thread_size = sizeof(struct rtos_thread) + stack_size;

	/* Do it */
	return _rtos2_alloc(thread_size);
}

__weak void _rtos2_release_thread(struct rtos_thread *thread)
{
	_rtos2_release(thread);
}

__weak void _rtos2_thread_stack_overflow(struct rtos_thread *thread)
{
	fprintf(stderr, "stack overflow: %s %p\n", thread->name, thread);
}

static void osSchedulerTaskEntryPoint(void *rtos_thread)
{
	/* Validate the thread */
	osStatus_t os_status = osIsResourceValid(rtos_thread, RTOS_THREAD_MARKER);
	if (os_status == osOK) {

		/* Update the task and the current task thread local */
		current_thread = rtos_thread;

		/* Forward */
		current_thread->func(current_thread->context);
	}

	/* Forward to thread exit */
	osThreadExit();
}

static void osSchedulerTaskExitHandler(struct task *task)
{
	assert(task != 0);

	/* Extract the matching thread */
	osStatus_t os_status = osIsResourceValid(task->context, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		abort();
	struct rtos_thread *thread = task->context;

	/* Check for overflow */
	if (task->psp->r0 == (uint32_t)-EFAULT)
		_rtos2_thread_stack_overflow(thread);

	/* Joinable? */
	if ((thread->attr_bits & osThreadJoinable)) {

		/* Let any joiner know about this */
		uint32_t flags = osEventFlagsSet(&thread->joiner, RTOS_THREAD_JOINED);
		if (flags & osFlagsError)
			abort();

		/* The joiner will handle the cleanup */
		return;
	}

	/* Detached thread, mark for reaping and kick the reaper */
	task->state = TASK_OTHER;
	uint32_t flags = osThreadFlagsSet(reaper_thread, RTOS_REAPER_CLEAN);
	if (flags & osFlagsError)
		abort();
}

osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
	const osThreadAttr_t default_attr = { .stack_size = RTOS_DEFAULT_STACK_SIZE, .priority = osPriorityNormal };

	/* Bail if not thread function provided */
	if (!func)
		return 0;

	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK) {
		errno = EINVAL;
		return 0;
	}

	/* Check for attribute */
	if (!attr)
		attr = &default_attr;

	/* Range check the thread priority */
	if (attr->priority < osPriorityNone || attr->priority > osPriorityISR) {
		errno = EINVAL;
		return 0;
	}

	/* Setup the thread memory */
	struct rtos_thread *new_thread = 0;
	size_t stack_size = 0;
	if (!attr->cb_mem && !attr->stack_mem) {

		/* We will need more room on the stack for book keeping */
		stack_size = osThreadMinimumStackSize + (attr->stack_size == 0 ? RTOS_DEFAULT_STACK_SIZE : attr->stack_size);

		/* Dynamic allocation */
		new_thread = _rtos2_alloc_thread(stack_size);
		if (!new_thread)
			return 0;

		/* Initialize the pointers */
		new_thread->stack = new_thread->stack_area;
		new_thread->stack_size = attr->stack_size == 0 ? RTOS_DEFAULT_STACK_SIZE : attr->stack_size;
		new_thread->attr_bits = attr->attr_bits | osDynamicAlloc;

	/* Static allocation */
	} else if (attr->cb_mem && attr->stack_mem) {

		/* Hopefully this will be enough room */
		stack_size = attr->stack_size;

		/* Make sure the stack size if reasonable */
		if (attr->cb_size < sizeof(struct rtos_thread) || stack_size < osThreadMinimumStackSize)  {
			errno = EINVAL;
			return 0;
		}

		/* Initialize the pointers */
		new_thread = attr->cb_mem;
		new_thread->stack = attr->stack_mem;
		new_thread->stack_size = attr->stack_size;
		new_thread->attr_bits = attr->attr_bits;

	/* Static memory allocation is all or nothing */
	} else {
		errno = EINVAL;
		return 0;
	}

	/* Initialize the remain parts of the thread */
	new_thread->name = attr->name;
	new_thread->marker = RTOS_THREAD_MARKER;
	new_thread->func = func;
	new_thread->context = argument;
	list_init(&new_thread->resource_node);

	/* Initialized the the thread flags */
	osEventFlagsAttr_t eventflags_attr = { .name = attr->name, .cb_mem = &new_thread->flags, .cb_size = sizeof(struct rtos_eventflags) };
	if (!osEventFlagsNew(&eventflags_attr))
		goto delete_thread;

	/* Initialized the the joiner event flags */
	eventflags_attr.cb_mem = &new_thread->joiner;
	if (!osEventFlagsNew(&eventflags_attr))
		goto delete_flags;

	/* Assemble the scheduler task descriptor */
	struct task_descriptor desc;
	strncpy(desc.name, (attr->name ? attr->name : "rtos-toolkit"), TASK_NAME_LEN);
	desc.entry_point = osSchedulerTaskEntryPoint;
	desc.exit_handler = osSchedulerTaskExitHandler;
	desc.context = new_thread;
	desc.flags = SCHEDULER_TASK_STACK_CHECK;
	desc.priority = osSchedulerPriority(attr->priority == osPriorityNone ? osPriorityNormal : attr->priority);

	/* Add it to the kernel thread resource list */
	os_status = osKernelResourceAdd(osResourceThread, &new_thread->resource_node);
	if (os_status != osOK)
		goto delete_joiner;

	/* Launch the thread, the entry point handler will complete the initialization */
	if (!scheduler_create(new_thread->stack, stack_size, &desc))
		goto remove_resource;

	/* Joy */
	return new_thread;

remove_resource:
	osKernelResourceRemove(osResourceThread, &new_thread->resource_node);

delete_joiner:
	osEventFlagsDelete(&new_thread->joiner);

delete_flags:
	osEventFlagsDelete(&new_thread->flags);

delete_thread:
	if (new_thread->attr_bits & osDynamicAlloc)
		_rtos2_release_thread(new_thread);

	/* The big fail */
	return 0;
}

const char *osThreadGetName(osThreadId_t thread_id)
{
	/* Check the thread id is provided */
	struct rtos_thread *thread = (struct rtos_thread *)thread_id;
	if (!thread)
		return 0;

	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return 0;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return 0;

	/* Return the name */
	return thread->name;
}

osThreadId_t osThreadGetId(void)
{
	return current_thread;
}

osThreadState_t osThreadGetState(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return osThreadError;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return osThreadError;
	struct rtos_thread *thread = thread_id;
	struct task *task = thread->stack;

	/* Remap the task states */
	switch (task->state) {

		case TASK_TERMINATED:
			return osThreadTerminated;

		case TASK_RUNNING:
			return osThreadRunning;

		case TASK_READY:
			return osThreadReady;

		case TASK_BLOCKED:
		case TASK_SUSPENDED:
		case TASK_SLEEPING:
			return osThreadBlocked;

		default:
			return osThreadError;
	}
}

uint32_t osThreadGetStackSize(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return 0;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return 0;
	struct rtos_thread *thread = thread_id;

	/* Looks ok */
	return thread->stack_size;
}

uint32_t osThreadGetStackSpace(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return 0;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return 0;
	struct rtos_thread *thread = thread_id;
	struct task *task = thread->stack;

	/* Lock the kernel so we the thread does not change */
	osKernelLock();
	uint32_t *current_pos = task->stack_marker;
	while (current_pos < task->stack_marker + (thread->stack_size / 4)) {
		if (*current_pos != SCHEDULER_STACK_MARKER)
			break;
		++current_pos;
	}
	osKernelUnlock();

	/* Return the unused stack */
	uint32_t unused_stack = (current_pos - task->stack_marker) * 4;
	return unused_stack;
}

osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority)
{
	/* Range Check the priority */
	if (priority < osPriorityIdle || priority > osPriorityISR)
		return osErrorParameter;

	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return osErrorISR;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = thread_id;

	/* Forward */
	int status = scheduler_set_priority(thread->stack, osSchedulerPriority(priority));
	if (status < 0)
		return osError;

	/* All done here */
	return osOK;
}

osPriority_t osThreadGetPriority(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return osPriorityError;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return osPriorityError;
	struct rtos_thread *thread = thread_id;

	/* Return the mapped priority */
	return osKernelPriority(scheduler_get_priority(thread->stack));
}

osStatus_t osThreadYield(void)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Forward to the scheduler */
	scheduler_yield();

	/* All good, we got processor back */
	return osOK;
}

osStatus_t osThreadSuspend(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = thread_id;

	/* Check the thread state */
	switch (osThreadGetState(thread_id)) {
		case osThreadReady:
		case osThreadRunning:
		case osThreadBlocked:
			break;

		default:
			return osErrorResource;
	}

	/* Can not suspend while locked, consider removing if needed, scheduler changes required but it can be made to work */
	if (thread_id == osThreadGetId() && rtos2_kernel->locked)
		return osError;

	/* Forward */
	int status = scheduler_suspend(thread->stack);
	if (status < 0)
		return osError;

	/* All good */
	return osOK;
}

osStatus_t osThreadResume(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = thread_id;

	/* Check the thread state */
	if (osThreadGetState(thread_id) != osThreadBlocked)
		return osErrorResource;

	/* Forward */
	int status = scheduler_resume(thread->stack);
	if (status < 0)
		return osError;

	/* Moving on now */
	return osOK;
}

osStatus_t osThreadDetach(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = thread_id;
	struct task *task = thread->stack;

	/* Make sure the thread is not already detached */
	if ((thread->attr_bits & osThreadJoinable) == 0)
		return osErrorResource;

	/* Block every while updating */
	uint32_t state = osKernelEnterCritical();
	thread->attr_bits &= ~osThreadJoinable;
	if (task->state == TASK_TERMINATED)
		task->state = TASK_OTHER;
	osKernelExitCritical(state);

	/* Reap the thread if already terminated */
	if (task->state == TASK_OTHER) {
		uint32_t flags = osThreadFlagsSet(reaper_thread, RTOS_REAPER_CLEAN);
		if (flags & 0x80000000)
			return (osStatus_t)flags;
	}

	/* All good */
	return osOK;
}

static osStatus_t osThreadReap(const osResource_t resource, void *context)
{
	/* Maker the resource is valid */
	osStatus_t os_status = osIsResourceValid(resource, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = resource;

	/* Release inactive threads */
	osThreadState_t thread_state = osThreadGetState(thread);
	if (thread_state == osThreadError) {

		/* Remove from resource list */
		osStatus_t os_status = osKernelResourceRemove(osResourceThread, &thread->resource_node);
		if (os_status != osOK)
			return os_status;

		/* Clear the marker */
		thread->marker = 0;

		/* Are we managing the memory? */
		if (thread->attr_bits & osDynamicAlloc)
			_rtos2_release_thread(thread);
	}

	/* All good */
	return osOK;
}

static void osThreadReaper(void *context)
{
	/* Allow kernel to exit even if we are still running */
	scheduler_set_flags(0, SCHEDULER_IGNORE_VIABLE);

	/* Cleanup until exit */
	while (true) {

		/* Wait for signal */
		uint32_t flags = osThreadFlagsWait(RTOS_REAPER_EXIT | RTOS_REAPER_CLEAN, osFlagsWaitAny, osWaitForever);
		if (flags & osFlagsError)
			abort();

		/* Reap some threads? */
		if (flags & RTOS_REAPER_CLEAN) {
			osStatus_t os_status = osKernelResourceForEach(osResourceThread, osThreadReap, 0);
			if (os_status != osOK)
				break;
		}

		/* Done? */
		if (flags & RTOS_REAPER_EXIT)
			break;
	}
}

static void osThreadReaperInit(void)
{
	/* Create the reaper thread */
	osThreadAttr_t attr = { .name = "osThreadReaper", .stack_size = RTOS_DEFAULT_STACK_SIZE, .priority = osPriorityNormal };
	reaper_thread = osThreadNew(osThreadReaper, 0, &attr);
	if (!reaper_thread)
		abort();
}

static osStatus_t osReleaseOwnRobustMutex(const osResource_t resource, void *context)
{
	/* Validate the mutex */
	osStatus_t os_status = osIsResourceValid(resource, RTOS_MUTEX_MARKER);
	if (os_status != osOK)
		return os_status;
	osMutexId_t mutex = resource;

	/* Validate the thread */
	os_status = osIsResourceValid(context, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	osThreadId_t owner = context;

	/* Release the mutex, handling and recursive locks */
	while (osMutexGetOwner(mutex) == owner) {
		os_status = osMutexRobustRelease(mutex, owner);
		if (os_status != osOK)
			abort();
	}

	/* Continue on */
	return osOK;
}

void osThreadExit(void)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		abort();

	/* Initialize the thread reaper if needed, i.e. the thread exiting is detached  */
	if ((current_thread->attr_bits & osThreadJoinable) == 0)
		osCallOnce(&reaper_thread_init, osThreadReaperInit);

	/* Release any robust mutexes we own */
	osKernelResourceForEach(osResourceRobustMutex, osReleaseOwnRobustMutex, osThreadGetId());

	/* Tell the scheduler to evict the task, cleaning happens on the reaper thread through the scheduler task exit handler callback */
	scheduler_terminate(0);

	/* We should never get here but scheduler_terminate is not __no_return */
	abort();
}

osStatus_t osThreadJoin(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = thread_id;

	/* Check the state */
	if (osThreadGetState(thread_id) == osThreadError)
		return osErrorParameter;

	/* Must be joinable */
	if ((thread->attr_bits & osThreadJoinable) == 0)
		return osErrorResource;

	/* Wait the the thread to terminate */
	uint32_t flags = osEventFlagsWait(&thread->joiner, RTOS_THREAD_JOINED, osFlagsWaitAny, osWaitForever);
	if (flags & osFlagsError)
		return (osStatus_t)flags;

	/* We own the terminated thread temove from resource list */
	os_status = osKernelResourceRemove(osResourceThread, &thread->resource_node);
	if (os_status != osOK)
		return os_status;

	/* Clear the marker */
	thread->marker = 0;

	/* Are we managing the memory? */
	if (thread->attr_bits & osDynamicAlloc)
		_rtos2_release_thread(thread);

	/* We own the terminating thread clean up */
	return osOK;
}

osStatus_t osThreadTerminate(osThreadId_t thread_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Validate the thread */
	os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = thread_id;

	/* Initialize the thread reaper if needed */
	if ((thread->attr_bits & osThreadJoinable) == 0)
		osCallOnce(&reaper_thread_init, osThreadReaperInit);

	/* Release any robust mutexes owned by the thread */
	osKernelResourceForEach(osResourceRobustMutex, osReleaseOwnRobustMutex, thread_id);

	/* Start the thread termination */
	int status = scheduler_terminate(thread->stack);
	if (status < 0)
		return osErrorResource;

	/* Clean should be in progress */
	return osOK;
}

static osStatus_t osCountThreads(const osResourceNode_t resource_node, void *context)
{
	if (!context)
		return osError;

	/* Only count active threads */
	osThreadState_t thread_state = osThreadGetState(resource_node);
	if (thread_state != osThreadError)
		*((int *)context) += 1;

	return osOK;
}

uint32_t osThreadGetCount(void)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return 0;

	/* Count them up */
	int count = 0;
	os_status = osKernelResourceForEach(osResourceThread, osCountThreads, &count);
	if (os_status != osOK)
		return 0;

	/* Done */
	return count;
}

static osStatus_t osCaptureThreads(const osResourceNode_t resource_node, void *context)
{
	/* Make sure we can work correctly */
	if (!context)
		return osError;

	/* Update the capture */
	struct rtos2_thread_capture *capture = context;
	if (capture->count < capture->size)
		capture->threads[capture->count++] = resource_node;

	/* Continue */
	return osOK;
}

uint32_t osThreadEnumerate(osThreadId_t *thread_array, uint32_t array_items)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return 0;

	/* Enumerate */
	struct rtos2_thread_capture capture = { .count = 0, .size = array_items, .threads = thread_array };
	os_status = osKernelResourceForEach(osResourceThread, osCaptureThreads, &capture);
	if (os_status != osOK)
		return os_status;

	/* Number thread ids stored */
	return capture.count;
}

uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags)
{
	/* Range check the flags */
	if (flags & osFlagsError)
		return osErrorParameter;

	/* Validate the thread */
	osStatus_t os_status = osIsResourceValid(thread_id, RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = thread_id;

	/* Forward */
	return osEventFlagsSet(&thread->flags, flags);
}

uint32_t osThreadFlagsClear(uint32_t flags)
{
	/* Validate the thread */
	osStatus_t os_status = osIsResourceValid(osThreadGetId(), RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = osThreadGetId();

	/* Forward */
	return osEventFlagsClear(&thread->flags, flags);
}

uint32_t osThreadFlagsGet(void)
{
	/* Validate the thread */
	osStatus_t os_status = osIsResourceValid(osThreadGetId(), RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = osThreadGetId();

	/* Forward */
	return osEventFlagsGet(&thread->flags);
}

uint32_t osThreadFlagsWait(uint32_t flags, uint32_t options, uint32_t timeout)
{
	/* Range check the flags */
	if (flags & osFlagsError)
		return osErrorParameter;

	/* Validate the thread */
	osStatus_t os_status = osIsResourceValid(osThreadGetId(), RTOS_THREAD_MARKER);
	if (os_status != osOK)
		return os_status;
	struct rtos_thread *thread = osThreadGetId();

	/* Forward */
	return osEventFlagsWait(&thread->flags, flags, options, timeout);
}