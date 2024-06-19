/*
 * Copyright (C) 2017 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * diag.c
 *
 * Created on: Mar 8, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <init/init-sections.h>

#include <sys/iob.h>
#include <sys/lock.h>

#include <diag/diag.h>

static int picolibc_diag_putc(char c, FILE *file)
{
	return diag_putc(c);
}

static int picolibc_diag_getc(FILE *file)
{
	return diag_getc();
}

static int picolibc_diag_flush(FILE *file)
{
	return 0;
}

static char diag_buffer[DIAG_BUFFER_SIZE];

int diag_vprintf(const char *fmt, va_list args)
{
	__retarget_lock_acquire_recursive(&__lock___libc_recursive_mutex);

	/* Visually mark buffer truncation */
	diag_buffer[DIAG_BUFFER_SIZE - 4] = '.';
	diag_buffer[DIAG_BUFFER_SIZE - 3] = '.';
	diag_buffer[DIAG_BUFFER_SIZE - 2] = '.';
	diag_buffer[DIAG_BUFFER_SIZE - 1] = 0;

	/* Build the buffer */
	ssize_t amount = vsnprintf(diag_buffer, DIAG_BUFFER_SIZE - 3, fmt, args);
	if (amount > DIAG_BUFFER_SIZE - 3)
		amount = DIAG_BUFFER_SIZE;

	/* Forward to link time selected output function */
	diag_puts(diag_buffer);

	__retarget_lock_release_recursive(&__lock___libc_recursive_mutex);

	return amount;
}

int diag_printf(const char *fmt, ...)
{
	va_list ap;

	/* Initialize the variable arguments */
	va_start(ap, fmt);

	int amount = diag_vprintf(fmt, ap);

	/* Clean up */
	va_end(ap);

	/* Return the amount of data actually sent */
	return amount;
}

static void diag_init(void)
{
	iob_setup_stream(&_stdio, picolibc_diag_putc, picolibc_diag_getc, picolibc_diag_flush, 0, _FDEV_SETUP_RW, 0);
	iob_setup_stream(&_stderr, picolibc_diag_putc, picolibc_diag_getc, picolibc_diag_flush, 0, _FDEV_SETUP_RW, 0);
	iob_setup_stream(&_stddiag, picolibc_diag_putc, picolibc_diag_getc, picolibc_diag_flush, 0, _FDEV_SETUP_RW, 0);
}
PREINIT_PLATFORM_WITH_PRIORITY(diag_init, DIAG_PLATFORM_INIT_PRIORITY);
