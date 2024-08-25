/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _POSIX_IO_H_
#define _POSIX_IO_H_

#include <config.h>

#include <sys/types.h>

#ifndef DEVICES_PRIORITY
#define DEVICES_PRIORITY 500
#endif

typedef int (*posix_device_open_t)(const char *path, int flags);

struct posix_ops
{
	int (*close)(int fd);
	ssize_t (*read)(int fd, void *buf, size_t count);
	ssize_t (*write)(int fd, const void *buf, size_t count);
	off_t (*lseek)(int fd, off_t offset, int whence);
};

int posix_register(const char *device_name, posix_device_open_t pdopen);
void posix_unregister(const char *device_name);

#endif
