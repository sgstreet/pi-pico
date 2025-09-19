/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/lock.h>
#include <sys/types.h>
#include <sys/syslog.h>

#include <devices/posix-io.h>

#define DEVICE_SLOT_GROWTH 10UL

struct posix_device
{
	const char *name;
	posix_device_open_t pdopen;
};

static struct posix_device *devices = 0;
static size_t max_slots = 0;
static size_t used_slots = 0;

static int posix_device_compare(const void *first, const void *second)
{
	const struct posix_device *first_device = first;
	const struct posix_device *second_device = second;

	return strncmp(first_device->name, second_device->name, strlen(second_device->name));
}

int open(const char *path, int oflag, ...)
{
	/* Check the parameters, we ignore the varagrs */
	if (path == 0 || strlen(path) == 0) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* Protect the device table */
	__retarget_lock_acquire_recursive(&__lock___libc_recursive_mutex);

	/* Look for the device name */
	struct posix_device key = { .name = path, .pdopen = 0 };
	struct posix_device *device = bsearch(&key, devices, used_slots, sizeof(struct posix_device), posix_device_compare);
	if (!device) {
		errno = ENODEV;
		goto error;
	}

	/* All done with the table */
	__retarget_lock_release_recursive(&__lock___libc_recursive_mutex);

	/* Forward to the open */
	return device->pdopen(path, oflag);

error:
	/* All done with the table */
	__retarget_lock_release_recursive(&__lock___libc_recursive_mutex);

	return -1;
}

int close(int fd)
{
//	/* Valid fd? */
//	if (fd <= 0) {
//		errno = EINVAL;
//		return -EINVAL;
//	}

	/* Forward */
	return ((const struct posix_ops *)fd)->close(fd);
}

ssize_t read(int fd, void *buf, size_t count)
{
//	/* Valid fd? */
//	if (fd <= 0) {
//		errno = EINVAL;
//		return -1;
//	}

	/* Forward */
	return ((const struct posix_ops *)fd)->read(fd, buf, count);
}

ssize_t write(int fd, const void *buf, size_t count)
{
//	/* Valid fd? */
//	if (fd <= 0) {
//		errno = EINVAL;
//		return -1;
//	}

	/* Forward */
	return ((const struct posix_ops *)fd)->write(fd, buf, count);
}

off_t lseek(int fd, off_t offset, int whence)
{
//	/* Valid fd? */
//	if (fd <= 0) {
//		errno = EINVAL;
//		return -1;
//	}

	/* Forward */
	return ((const struct posix_ops *)fd)->lseek(fd, offset, whence);
}

int posix_register(const char *device_name, posix_device_open_t pdopen)
{
	int status = 0;

	/* Check the arguments */
	if (device_name == 0 || strlen(device_name) == 0 || pdopen == 0) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* Protect the device table */
	__retarget_lock_acquire_recursive(&__lock___libc_recursive_mutex);

	/* Do we need more memory for the device table? */
	if (used_slots == max_slots) {

		struct posix_device *new_devices = reallocarray(devices, max_slots + DEVICE_SLOT_GROWTH, sizeof(struct posix_device));
		if (new_devices == 0) {
			status = -errno;
			goto error;
		}

		/* Update the array and the max slots */
		devices = new_devices;
		max_slots += DEVICE_SLOT_GROWTH;
	}

	/* Update the slot */
	devices[used_slots].name = device_name;
	devices[used_slots].pdopen = pdopen;
	++used_slots;

	/* Sort the table */
	qsort(devices, used_slots, sizeof(struct posix_device), posix_device_compare);

	syslog_info("%s\n", device_name);

error:
	/* All done with the table */
	__retarget_lock_release_recursive(&__lock___libc_recursive_mutex);

	/* Shoudl be good */
	return status;
}

void posix_unregister(const char *device_name)
{
	/* Check the arguments */
	if (device_name == 0 || strlen(device_name) == 0)
		return;

	syslog_info("%s\n", device_name);

	/* Need the lock to access the table */
	__retarget_lock_acquire_recursive(&__lock___libc_recursive_mutex);

	/* Look for matching name */
	size_t entry;
	for (entry = 0; entry < used_slots; ++entry)
		if (strcmp(device_name, devices[entry].name) == 0)
			break;

	/* If we found it, remove by shortening the array via copy */
	if (entry < used_slots) {
		memcpy(&devices[entry], &devices[entry + 1], (used_slots - entry - 1) * sizeof(struct posix_device));
		--used_slots;
	}

	/* All done with the table */
	__retarget_lock_release_recursive(&__lock___libc_recursive_mutex);
}
