/*
 * async.h
 *
 *  Created on: Oct 12, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _ASYNC_H_
#define _ASYNC_H_

#include <assert.h>
#include <stdatomic.h>


struct async
{
	void *context;
	void (*func)(struct async *async);
	atomic_bool done;
	atomic_bool cancel;
};

typedef void (*async_func_t)(struct async *async);

int async_run(struct async *async, async_func_t func, void *context);

bool async_is_done(struct async *async);
void async_done(struct async *async);
void async_wait(struct async *async);

bool async_is_canceled(struct async *async);
void async_cancel(struct async *async);

#endif
