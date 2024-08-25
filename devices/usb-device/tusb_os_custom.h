/*
 * tusb_os_custom.h
 *
 *  Created on: Nov 29, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _TUSB_OS_CUSTOM_H_
#define _TUSB_OS_CUSTOM_H_

#include <rtos/rtos.h>

typedef osSemaphoreId_t osal_semaphore_t;
typedef osMutexId_t osal_mutex_t;
typedef osMessageQueueId_t osal_queue_t;

typedef osSemaphoreAttr_t osal_semaphore_def_t;
typedef osMutexAttr_t osal_mutex_def_t;

typedef struct {
	uint32_t msg_count;
	uint32_t msg_size;
	osMessageQueueAttr_t attr;
} osal_queue_def_t;

#define OSAL_QUEUE_DEF(role, _name, _depth, _type) \
	osal_queue_def_t _name = \
	{ \
		.msg_count = _depth, \
		.msg_size = sizeof(_type), \
		.attr = { \
			.name = #_name, \
		}, \
	}

static inline osal_semaphore_t osal_semaphore_create(osal_semaphore_def_t* semdef)
{
	return osSemaphoreNew(UINT32_MAX, 0, semdef);
}

static inline bool osal_semaphore_post(osal_semaphore_t sem_hdl, bool in_isr)
{
	return osSemaphoreRelease(sem_hdl) == osOK;
}

static inline bool osal_semaphore_wait(osal_semaphore_t sem_hdl, uint32_t msec)
{
	return osSemaphoreAcquire(sem_hdl, msec);
}

static inline osal_mutex_t osal_mutex_create(osal_mutex_def_t* mdef)
{
	return osMutexNew(mdef);
}

static inline bool osal_mutex_lock (osal_mutex_t mutex_hdl, uint32_t msec)
{
	return osMutexAcquire(mutex_hdl, msec) == osOK;
}

static inline bool osal_mutex_unlock(osal_mutex_t mutex_hdl)
{
	return osMutexRelease(mutex_hdl) == osOK;
}

static inline osal_queue_t osal_queue_create(osal_queue_def_t* qdef)
{
	return osMessageQueueNew(qdef->msg_count, qdef->msg_size, &qdef->attr);
}

static inline bool osal_queue_receive(osal_queue_t qhdl, void* data, uint32_t msec)
{
	return osMessageQueueGet(qhdl, data, 0, msec) == osOK;
}

static inline bool osal_queue_send(osal_queue_t qhdl, void const * data, bool in_isr)
{
	return osMessageQueuePut(qhdl, data, 0, in_isr ? 0 : osWaitForever) == osOK;
}

static inline bool osal_queue_empty(osal_queue_t qhdl)
{
	return osMessageQueueGetCount(qhdl) == 0;
}

#endif
