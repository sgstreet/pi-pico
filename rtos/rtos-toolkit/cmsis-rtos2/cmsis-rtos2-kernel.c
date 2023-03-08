#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <init/init-sections.h>

#include <rtos/rtos-toolkit/rtos-toolkit.h>

struct rtos_kernel *rtos2_kernel = 0;

extern __weak void *_rtos2_alloc(size_t size);
extern __weak void _rtos2_release(void *ptr);
extern void *__tls_size;

__weak void *_rtos2_alloc(size_t size)
{
	/* By default we use the monotonic allocator */
	void *ptr = sbrk(size);
	if (!ptr)
		return 0;

	/* Clear the allocation */

	return memset(ptr, 0, size);
}

__weak void _rtos2_release(void *ptr)
{
}

osStatus_t osKernelInitialize(void)
{
	static struct rtos_kernel kernel = { .state = osKernelInactive };

	const char *resource_names[] =
	{
		"thread",
		"mutex",
		"robust_mutex",
		"memory_pool",
		"semaphore",
		"eventflags",
		"timer",
		"message_queue",
		"ring_buffer",
	};
	const size_t resource_offsets[] =
	{
		offsetof(struct rtos_thread, resource_node),
		offsetof(struct rtos_mutex, resource_node),
		offsetof(struct rtos_mutex, resource_node),
		offsetof(struct rtos_memory_pool, resource_node),
		offsetof(struct rtos_semaphore, resource_node),
		offsetof(struct rtos_eventflags, resource_node),
		offsetof(struct rtos_timer, resource_node),
		offsetof(struct rtos_message_queue, resource_node),
		offsetof(struct rtos_deque, resource_node)
	};
	const osResourceMarker_t resource_markers[] =
	{
		RTOS_THREAD_MARKER,
		RTOS_MUTEX_MARKER,
		RTOS_MEMORY_POOL_MARKER,
		RTOS_SEMAPHORE_MARKER,
		RTOS_EVENTFLAGS_MARKER,
		RTOS_TIMER_MARKER,
		RTOS_MESSAGE_QUEUE_MARKER,
		RTOS_DEQUE_MARKER,
	};

	/* Can not be called from interrupt context */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Are we already initialized? */
	if (rtos2_kernel)
		return osError;

	/* Initialize the scheduler */
	int status = scheduler_init(&kernel.scheduler, (size_t)&__tls_size);
	if (status < 0)
		return osError;

	/* Initialize the resource lists, manually initialize lock to prevent initialization loops */
	for (osResourceId_t i = osResourceThread; i < osResourceLast; ++i) {

		/* Get the resource */
		struct rtos_resource *resource = &kernel.resources[i];

		/* Manual initialization of the mutex */
		resource->resource_lock.marker = RTOS_MUTEX_MARKER;
		resource->resource_lock.name = "thread_list_lock";
		resource->resource_lock.attr_bits = osMutexPrioInherit | osMutexRecursive | osMutexRobust;
		resource->resource_lock.value = 0;
		scheduler_futex_init(&resource->resource_lock.futex, (long *)&resource->resource_lock.value, SCHEDULER_FUTEX_PI | SCHEDULER_FUTEX_OWNER_TRACKING | SCHEDULER_FUTEX_CONTENTION_TRACKING);
		resource->resource_lock.count = 0;
		list_init(&resource->resource_lock.resource_node);

		/* Initialize basics stuff */
		resource->marker = resource_markers[i];
		resource->name = resource_names[i];
		resource->offset = resource_offsets[i];
		list_init(&resource->resource_list);
	}

	/* Add all the resource locks to the robust mutex resource list */
	for (int i = osResourceThread; i < osResourceLast; ++i)
		list_add(&kernel.resources[osResourceRobustMutex].resource_list, &kernel.resources[i].resource_lock.resource_node);

	/* Mark the kernel as initialized */
	kernel.state = osKernelReady;
	rtos2_kernel = &kernel;

	/* All good */
	return osOK;
}

osStatus_t osKernelGetInfo(osVersion_t *version, char *id_buf, uint32_t id_size)
{

	snprintf(id_buf, id_size, "rtos-toolkit");
	id_buf[id_size - 1] = 0;
	version->api = 02001003;
	version->kernel = 02001003;

	return osOK;
}

osKernelState_t osKernelGetState(void)
{
	/* Has the kernel been initialized? */
	if (!rtos2_kernel)
		return osKernelInactive;

	/* Return the reported state */
	return rtos2_kernel->state;
}

osStatus_t osKernelStart(void)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Can not start an uninitialized kernel */
	if (!rtos2_kernel || rtos2_kernel->state != osKernelReady)
		return osError;

	/* Start up the scheduler, we return here when the scheduler terminates */
	rtos2_kernel->state = osKernelRunning;
	int status = scheduler_run();

	/* Scheduler is down now */
	return status == 0 ? osOK : osError;
}

int32_t osKernelLock(void)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Kernel must be running or locked */
	if (rtos2_kernel->state != osKernelRunning && rtos2_kernel->state != osKernelLocked)
		return osError;

	/* Need a critical section to handle adaption */
	unsigned long state = scheduler_enter_critical();
	int32_t prev_lock = rtos2_kernel->locked;
	rtos2_kernel->locked = true;
	rtos2_kernel->state = osKernelLocked;
	if (!prev_lock)
		scheduler_lock();
	scheduler_exit_critical(state);

	/* Previous state */
	return prev_lock;
}

int32_t osKernelUnlock(void)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Kernel must be running or locked */
	if (rtos2_kernel->state != osKernelRunning && rtos2_kernel->state != osKernelLocked)
		return osError;

	/* Need a critical section to handle adaption */
	unsigned long state = scheduler_enter_critical();
	int32_t prev_lock = rtos2_kernel->locked;
	rtos2_kernel->locked = false;
	rtos2_kernel->state = osKernelRunning;
	if (prev_lock)
		scheduler_unlock();
	scheduler_exit_critical(state);

	/* Return the previous lock state */
	return prev_lock;
}

int32_t osKernelRestoreLock(int32_t lock)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Kernel must be running or locked */
	if (rtos2_kernel->state != osKernelRunning && rtos2_kernel->state != osKernelLocked)
		return osError;

	/* Need a critical section to handle adaption */
	unsigned long state = scheduler_enter_critical();
	rtos2_kernel->locked = lock;
	if (lock) {
		scheduler_lock();
		rtos2_kernel->state = osKernelLocked;
	} else {
		scheduler_unlock();
		rtos2_kernel->state = osKernelRunning;
	}
	scheduler_exit_critical(state);

	/* Return the previous lock state */
	return rtos2_kernel->locked;
}

uint32_t osKernelSuspend(void)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Bail if we are already suspended */
	if (rtos2_kernel->state == osKernelSuspended)
		return 0;

	/* Change the state */
	rtos2_kernel->state = osKernelSuspended;

	/* For now */
	return osWaitForever;
}

void osKernelResume(uint32_t sleep_ticks)
{
	/* This would be bad, drop the request */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return;

	rtos2_kernel->state = osKernelRunning;
}

uint32_t osKernelGetTickCount(void)
{
	return (uint32_t)(scheduler_jiffies() & 0xffffffff);
}

uint32_t osKernelGetTickFreq(void)
{
	return SCHEDULER_TICK_FREQ;
}

uint32_t osKernelGetSysTimerCount(void)
{
	uint32_t load = SysTick->LOAD;
	uint32_t sys_ticks = load - SysTick->VAL;
	return sys_ticks + osKernelGetTickCount() * (load + 1);
}

uint32_t osKernelGetSysTimerFreq(void)
{
	return SystemCoreClock;
}

void osCallOnce(osOnceFlagId_t once_flag, osOnceFunc_t func)
{
	/* All ready done */
	if (*once_flag == 2)
		return;

	/* Try to claim the initializer */
	int expected = 0;
	if (!atomic_compare_exchange_strong(once_flag, &expected, 1)) {

		/* Wait the the initializer to complete, we sleep to ensure lower priority threads run */
		while (*once_flag != 2)
			osDelay(10);

		/* Done */
		return;
	}

	/* Run the function */
	func();

	/*  Mark as done */
	*once_flag = 2;
}

osStatus_t osKernelResourceAdd(osResourceId_t resource_id, osResourceNode_t node)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Make a node was provided */
	if (!node) {
		errno = EINVAL;
		return osErrorParameter;
	}

	/* Get the resource from the kernel */
	struct rtos_resource *resource = &rtos2_kernel->resources[resource_id];

	/* Lock the resource, skip is kernel is not running */
	if (osKernelGetState() == osKernelRunning) {
		os_status = osMutexAcquire(&resource->resource_lock, osWaitForever);
		if (os_status != osOK)
			return os_status;
	}

	/* Add it */
	list_add(&resource->resource_list, node);


	/* Release the resource reporting any errors, if the kernel is running */
	if (osKernelGetState() != osKernelRunning)
		return osOK;

	/* Need the release the mutex */
	return osMutexRelease(&resource->resource_lock);
}

osStatus_t osKernelResourceRemove(osResourceId_t resource_id, osResourceNode_t node)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Make a node was provided */
	if (!node)
		return osErrorParameter;

	/* Get the resource from the kernel */
	struct rtos_resource *resource = &rtos2_kernel->resources[resource_id];

	/* Lock the resource, skip is kernel is not running */
	if (osKernelGetState() == osKernelRunning) {
		os_status = osMutexAcquire(&resource->resource_lock, osWaitForever);
		if (os_status != osOK)
			return os_status;
	}

	/* Remove it */
	list_remove(node);

	/* Release the resource reporting any errors, if the kernel is running */
	if (osKernelGetState() != osKernelRunning)
		return osOK;

	/* Need the release the mutex */
	return osMutexRelease(&resource->resource_lock);
}

bool osKernelResourceIsLocked(osResourceId_t resource_id)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	struct rtos_resource *resource = &rtos2_kernel->resources[resource_id];

	return resource->resource_lock.value != 0;
}

osStatus_t osKernelResourceForEach(osResourceId_t resource_id, osResouceNodeForEachFunc_t func, void *context)
{
	/* This would be bad */
	osStatus_t os_status = osKernelContextIsValid(false, 0);
	if (os_status != osOK)
		return os_status;

	/* Make a node was provided */
	if (!func)
		return osErrorParameter;

	/* Get the resource from the kernel */
	struct rtos_resource *resource = &rtos2_kernel->resources[resource_id];

	/* Lock the resource, skip is kernel is not running */
	if (osKernelGetState() == osKernelRunning) {
		os_status = osMutexAcquire(&resource->resource_lock, osWaitForever);
		if (os_status != osOK)
			return os_status;
	}

	/* Loop through all the node until either the end of the list or function return interesting status */
	osStatus_t func_status;
	struct linked_list *current;
	struct linked_list *next;
	list_for_each_mutable(current, next, &resource->resource_list) {
		osResource_t entry = ((void *)current) - resource->offset;
		func_status = func(entry, context);
		if (func_status != osOK)
			break;
	}

	/* Release the resource reporting any errors, if the kernel is running */
	if (osKernelGetState() == osKernelRunning) {
		os_status = osMutexRelease(&resource->resource_lock);
		if (os_status != osOK)
			return os_status;
	}

	/* Return the call back status */
	return func_status;
}

static osStatus_t osKernelDumpResource(osResource_t resource, void *context)
{
	/* Valid the resource */
	osResourceMarker_t marker = (osResourceMarker_t)context;
	osStatus_t os_status = osIsResourceValid(resource, marker);
	if (os_status != osOK)
		return os_status;

	/* Dump the resource */
	switch(marker) {

		case RTOS_THREAD_MARKER:
		{
			struct rtos_thread *thread = resource;
			fprintf(stdout, "thread: %p name: %s, state: %d stack available: %lu\n", thread, thread->name, osThreadGetState(thread), osThreadGetStackSpace(thread));
			break;
		}

		case RTOS_MUTEX_MARKER:
		case RTOS_MEMORY_POOL_MARKER:
		case RTOS_SEMAPHORE_MARKER:
		case RTOS_EVENTFLAGS_MARKER:
		case RTOS_TIMER_MARKER:
		case RTOS_MESSAGE_QUEUE_MARKER:
		case RTOS_DEQUE_MARKER:
		default:
			break;
	}

	/* Continue on */
	return osOK;
}

osStatus_t osKernelResourceDump(osResourceId_t resource_id)
{
	/* Check the kernel state */
	if (rtos2_kernel == 0 || rtos2_kernel->state != osKernelRunning) {
		errno = EINVAL;
		return osErrorResource;
	}

	osResourceMarker_t marker = 0;
	switch (resource_id) {
		case osResourceThread:
			marker = RTOS_THREAD_MARKER;
			break;
		case osResourceMutex:
			marker = RTOS_MUTEX_MARKER;
			break;
		case osResourceMemoryPool:
			marker = RTOS_MEMORY_POOL_MARKER;
			break;
		case osResourceSemaphore:
			marker = RTOS_SEMAPHORE_MARKER;
			break;
		case osResourceEventFlags:
			marker = RTOS_EVENTFLAGS_MARKER;
			break;
		case osResourceTimer:
			marker = RTOS_TIMER_MARKER;
			break;
		case osResourceMessageQueue:
			marker = RTOS_MESSAGE_QUEUE_MARKER;
			break;
		case osResourceDeque:
			marker = RTOS_DEQUE_MARKER;
			break;
		default:
			return osErrorParameter;
	}

	/* Run the iterator */
	return osKernelResourceForEach(resource_id, osKernelDumpResource, (void *)marker);
}

struct arguments
{
	int argc;
	char **argv;
	int ret;
};

extern int main(int argc, char **argv);
extern __weak void main_task(void *context);
extern void _init(void);
extern void _fini(void);

int _main(int argc, char **argv);

__weak void main_task(void *context)
{
	assert(context != 0);

	struct arguments *args = context;

	/* Common initialization */
	_init();

	/* Execute the ini array */
	run_init_array(__init_array_start, __init_array_end);

	args->ret = main(args->argc, args->argv);

	/* Execute the fini array */
	run_fini_array(__fini_array_start, __fini_array_end);

	/* Common clean up */
	_fini();
}

int _main(int argc, char **argv)
{
	/* Initialize the kernel */
	osStatus_t os_status = osKernelInitialize();
	if (os_status != osOK)
		return -errno;

	/* Setup the main task */
	struct arguments args = { .argc = argc, .argv = argv, .ret = 0 };
	osThreadAttr_t main_thread_attr = { .cb_mem = 0 };
	main_thread_attr.name = "main-task";
	main_thread_attr.attr_bits = osThreadDetached;
	main_thread_attr.cb_mem = alloca(SCHEDULER_MAIN_STACK_SIZE + sizeof(struct rtos_thread));
	main_thread_attr.cb_size = sizeof(struct rtos_thread);
	main_thread_attr.stack_mem = ((struct rtos_thread*)(main_thread_attr.cb_mem))->stack_area;
	main_thread_attr.stack_size = SCHEDULER_MAIN_STACK_SIZE;
	main_thread_attr.priority = osPriorityNormal;
	osThreadId_t main_task_id = osThreadNew(main_task, &args, &main_thread_attr);
	if (!main_task_id)
		return -EINVAL;

	/* Run the kernel */
	int status = 0;
	os_status = osKernelStart();
	if (os_status == osOK)
		status = args.ret;
	else
		status = -errno;

	return status;
}

error_t errno_from_rtos(int rtos)
{
	switch (rtos) {
		case osErrorTimeout:
			return ETIMEDOUT;
		case osErrorResource:
			return ERESOURCE;
		case osErrorParameter:
			return EINVAL;
		case osErrorNoMemory:
			return ENOMEM;
		case osErrorISR:
			return ENOTSUP;
		default:
			return ERTOS;
	}
}
