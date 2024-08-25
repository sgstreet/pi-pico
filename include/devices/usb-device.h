/*
 * usb-device.h
 *
 *  Created on: Dec 1, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include <container-of.h>

#include <devices/posix-io.h>

#ifndef USBD_TIMEOUT
#define USBD_TIMEOUT osWaitForever
#endif

struct usb_device
{
	uint32_t timeout;
	osThreadId_t task;
	atomic_bool run;
	struct ref ref;
};

struct usb_device_file
{
	struct usb_device *device;
	struct posix_ops ops;
};

int usb_device_ini(struct usb_device *usbd);
void usb_device_fini(struct usb_device *usbd);

struct usb_device *usb_device_create(void);
void usb_device_destroy(struct usb_device *usbd);

static inline struct usb_device *usb_device_from_fd(int fd)
{
	struct usb_device_file *file = container_of_or_null((struct posix_ops *)fd, struct usb_device_file, ops);
	return file ? file->device : 0;
}

#endif
