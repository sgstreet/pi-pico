/*
 * Copyright (C) 2017 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * semihost.h
 *
 * Created on: Mar 16, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef SEMIHOST_H_
#define SEMIHOST_H_

#include <sys/types.h>
#include <stdint.h>

#define SEMIHOST_SYS_OPEN 0x01UL
#define SEMIHOST_SYS_CLOSE 0x02UL
#define SEMIHOST_SYS_WRITEC 0x03UL
#define SEMIHOST_SYS_WRITE0 0x04UL
#define SEMIHOST_SYS_WRITE 0x05UL
#define SEMIHOST_SYS_READ 0x06UL
#define SEMIHOST_SYS_READC 0x07UL
#define SEMIHOST_SYS_ISERROR 0x08UL
#define SEMIHOST_SYS_ISTTY 0x09UL
#define SEMIHOST_SYS_SEEK 0x0AUL
#define SEMIHOST_SYS_FLEN 0x0CUL
#define SEMIHOST_SYS_TMPNAM 0x0DUL
#define SEMIHOST_SYS_REMOVE 0x0EUL
#define SEMIHOST_SYS_RENAME 0x0FUL
#define SEMIHOST_SYS_CLOCK 0x10UL
#define SEMIHOST_SYS_TIME 0x11UL
#define SEMIHOST_SYS_SYSTEM 0x12UL
#define SEMIHOST_SYS_ERRNO 0x13UL
#define SEMIHOST_SYS_GET_CMDLINE 0x15UL
#define SEMIHOST_SYS_HEAPINFO 0x16UL
#define SEMIHOST_SYS_ENTER_SVC 0x17UL
#define SEMIHOST_SYS_REPORT_EXCEPTION 0x18UL
#define SEMIHOST_SYS_ELAPSED 0x30UL
#define SEMIHOST_SYS_TICKFREQ 0x31UL

#define SEMIHOST_OPEN_R 0
#define SEMIHOST_OPEN_RW 2

#define SEMIHOST_OPEN_W 4
#define SEMIHOST_OPEN_WRT 6

#define SEMIHOST_OPEN_A 8
#define SEMIHOST_OPEN_RAC 10

int semihost_call(int code, void *data);

int semihost_is_attached(void);
int semihost_open(const char *path, uint32_t flags);
int semihost_close(int handle);
int semihost_read(int handle, void *buffer, size_t count);
int semihost_write(int handle, const void *buffer, size_t count);

void semihost_puts(const char *s);
int semihost_getc(void);
void semihost_putc(int c);

int semihost_is_tty(int handle);
int semihost_errno(void);

#endif /* SEMIHOST_H_ */
