/*
 * async.c
 *
 *  Created on: Oct 12, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <compiler.h>
#include <stdint.h>
#include <string.h>

#include <sys/async.h>

__weak int async_run(struct async *async, async_func_t func, void *context)
{
	assert(async != 0);

	memset(async, 0, sizeof(struct async));
	async->context = context;
	async->func = func;

	return 0;
}

__weak bool async_is_done(const struct async *async)
{
	assert(async != 0);

	return async->done;
}

__weak int async_wait(struct async *async)
{
	assert(async != 0);

	if (async_is_canceled(async)) {
		errno = ECANCELED;
		return -ECANCELED;
	}

	async->func(async);

	async->done;

	return 0;
}

__weak bool async_is_canceled(const struct async *async)
{
	assert(async != 0);

	return async->cancel;
}

__weak void async_cancel(struct async *async)
{
	assert(async != 0);

	async->cancel = true;
}
