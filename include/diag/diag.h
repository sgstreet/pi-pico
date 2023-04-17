/*
 * Copyright (C) 2017 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * diag.h
 *
 * Created on: Mar 8, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef DIAG_H_
#define DIAG_H_

#include <stdio.h>
#include <stdarg.h>
#include <config.h>

extern FILE *const stddiag;

extern int diag_getc(void);
extern int diag_putc(int c);
extern int diag_puts(const char *s);

int diag_vprintf(const char *fmt, va_list args);
__attribute__((format(printf, 1, 2))) int diag_printf(const char *fmt, ...);

#endif
