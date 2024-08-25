/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <config.h>
#include <ref.h>

#include <sys/syslog.h>

#include <hardware/rp2040/exti.h>
#include <rtos/rtos.h>
#include <board/board.h>

#include <svc/event-bus.h>
#include <devices/usb-device.h>

#include <tusb.h>

static osOnceFlag_t device_init_flag = osOnceFlagsInit;
static struct usb_device *device = 0;

static void vbus_detected_handler(EXTIn_Type exti, uint32_t state, void *context)
{
	assert(context != 0);

	/* Pi Pico wired badly, handle VBUS detection in GPIO interrupt */
	if (state == EXTI_LEVEL_HIGH)
		set_bit(&USBCTRL_REGS->USB_PWR, USBCTRL_REGS_USB_PWR_VBUS_DETECT_Pos);
	else
		clear_bit(&USBCTRL_REGS->USB_PWR, USBCTRL_REGS_USB_PWR_VBUS_DETECT_Pos);
}

void tud_mount_cb(void)
{
	syslog_info("device connected\n");
	board_led_on(0);
	event_post(EVENT_USB_STATE_CHANGED, true);
}

void tud_umount_cb(void)
{
	syslog_info("device disconnected\n");
	board_led_off(0);
	event_post(EVENT_USB_STATE_CHANGED, false);
}

static void usb_task(void *context)
{
	assert(context != 0);

	struct usb_device *device = context;

	/* Register for vbus detection interrupts */

	syslog_info("%s running\n", osThreadGetName(osThreadGetId()));

	tud_connect();

	/* While we are running, forward into the tiny usb stack, cleanup unblocks the tud task */
	while (device->run)
		tud_task_ext(device->timeout, false);

	tud_disconnect();

	syslog_info("%s exiting\n", osThreadGetName(osThreadGetId()));
}

int usb_device_ini(struct usb_device *usbd)
{
	assert(usbd != 0);

	/* Basic initialization */
	usbd->timeout = osWaitForever;
	usbd->run = true;

	/* Track VBUS */
	exti_register(EXTIn_CORE0_24, EXTI_BOTH_EDGE, INTERRUPT_NORMAL, vbus_detected_handler, usbd);

	/* Initialize the tinyusb stack */
	tud_init(BOARD_TUD_RHPORT);

	/* Initialize the VBUS detection state */
	if (SIO->GPIO_IN & (1UL << BOARD_USBD_VBUS_DETECT_GPIO)) {
		set_bit(&USBCTRL_REGS->USB_PWR, USBCTRL_REGS_USB_PWR_VBUS_DETECT_Pos);
		board_led_on(0);
	} else {
		clear_bit(&USBCTRL_REGS->USB_PWR, USBCTRL_REGS_USB_PWR_VBUS_DETECT_Pos);
		board_led_off(0);
	}
	set_bit(&USBCTRL_REGS->USB_PWR, USBCTRL_REGS_USB_PWR_VBUS_DETECT_OVERRIDE_EN_Pos);

	/* Enable VBUS tracking */
	exti_enable(EXTIn_CORE0_24);

	/* Start the usb task */
	osThreadAttr_t thread_attr = { .name = "usbd-task", .attr_bits = osThreadJoinable, .priority = osPriorityAboveNormal };
	usbd->task = osThreadNew(usb_task, usbd, &thread_attr);
	if (!usbd->task)
		syslog_fatal("failed to create the USB device task: %d\n", errno);

	/* All good */
	return 0;
}

void usb_device_fini(struct usb_device *usbd)
{
	assert(usbd != 0);

	/* Disable VBUS Tracking */
	exti_disable(EXTIn_CORE0_24);

	/* Disable the connection indicator */
	board_led_off(0);

	/* Terminate the task */
	usbd->run = false;
	tud_unblock(false);

	/* Join with terminate task */
	osStatus_t os_status = osThreadJoin(usbd->task);
	if (os_status != osOK)
		syslog_fatal("failed to join the usb task: %d\n", os_status);

	/* No need for the exti now */
	exti_unregister(EXTIn_CORE0_24, vbus_detected_handler);
}

struct usb_device *usb_device_create(void)
{
	/* Allocate some space */
	struct usb_device *usbd = malloc(sizeof(*usbd));
	if (!usbd)
		return 0;

	/* Forward */
	int status = usb_device_ini(usbd);
	if (status < 0) {
		free(usbd);
		return 0;
	}

	/* All good */
	return usbd;
}

void usb_device_destroy(struct usb_device *usbd)
{
	assert(usbd != 0);

	/* Forward */
	usb_device_fini(usbd);

	/* Clean up */
	free(usbd);
}

static void usb_device_release(struct ref *ref)
{
	assert(ref != 0);

	struct usb_device *usbd = device;
	device = 0;

	device_init_flag = osOnceFlagsInit;

	free(usbd);
}

static void usbd_device_init(osOnceFlagId_t flag_id, void *context)
{
	/* Try to create the device */
	device = usb_device_create();
	if (!device)
		syslog_fatal("failed to create USBD: %d\n", errno);

	/* Initialize the reference count */
	ref_ini(&device->ref, usb_device_release, 0);
}

static struct usb_device *usb_device_get(void)
{
	/* Setup the matching cdc serial device */
	osCallOnce(&device_init_flag, usbd_device_init, 0);

	/* Update the count */
	ref_up(&device->ref);

	/* Add return it */
	return device;
}

static void usb_device_put(struct usb_device *usbd)
{
	assert(usbd != 0);

	ref_down(&usbd->ref);
}

static int usb_device_posix_close(int fd)
{
	struct usb_device_file *file = container_of((struct posix_ops *)fd, struct usb_device_file, ops);

	/* Release reference to the device */
	usb_device_put(file->device);

	/* Release the uart serial file */
	free(file);

	/* Always good */
	return 0;
}

static ssize_t usb_device_posix_read(int fd, void *buf, size_t count)
{
	errno = ENOTSUP;
	return -1;
}

static ssize_t usb_device_posix_write(int fd, const void *buf, size_t count)
{
	errno = ENOTSUP;
	return -1;
}

static off_t usb_device_posix_lseek(int fd, off_t offset, int whence)
{
	errno = ENOTSUP;
	return (off_t)-1;
}

static int usb_device_open(const char *path, int flags)
{
	/* Check the path */
	if (path == 0 || strlen(path) < strlen("USBD")) {
		errno = EINVAL;
		return -1;
	}

	/* Allocate the uart serial channel */
	struct usb_device_file *file = calloc(1, sizeof(struct usb_device_file));
	if (!file) {
		errno = ENOMEM;
		return -ENOMEM;
	}

	/* Get the underlaying uart device */
	file->device = usb_device_get();

	/* Setup the posix interface */
	file->ops.close = usb_device_posix_close;
	file->ops.read = usb_device_posix_read;
	file->ops.write = usb_device_posix_write;
	file->ops.lseek = usb_device_posix_lseek;

	/* Return the ops as a integer */
	return (intptr_t)&file->ops;
}

static __constructor_priority(DEVICES_PRIORITY) void usb_device_posix_ini(void)
{
	int status = posix_register("USBD", usb_device_open);
	if (status != 0)
		syslog_fatal("failed to register posix device: %d\n", errno);
}

static __destructor_priority(DEVICES_PRIORITY) void usb_device_posix_fini(void)
{
	posix_unregister("USBD");
}
