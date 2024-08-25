/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <ref.h>
#include <config.h>

#include <sys/syslog.h>

#include <rtos/rtos.h>

#include <class/cdc/cdc_device.h>

#include <svc/event-bus.h>

#include <devices/io-ring.h>
#include <devices/posix-io.h>
#include <devices/wait-queue.h>
#include <devices/cdc-serial.h>
#include <devices/usb-device.h>

static osOnceFlag_t device_init_flags[CFG_TUD_CDC] = { [0 ... CFG_TUD_CDC - 1] = osOnceFlagsInit };
static struct cdc_serial *devices[CFG_TUD_CDC] = { [0 ... CFG_TUD_CDC - 1] = 0 };

static void tud_cdc_rx_drop(uint8_t itf, size_t amount)
{
	uint32_t data;
	while (amount > 0)
		amount -= tud_cdc_n_read(itf, &data, sizeof(data));
}

void tud_cdc_rx_cb(uint8_t itf)
{
	size_t amount = tud_cdc_n_available(itf);

	/* If not initialized just consume the received data */
	if (!devices[itf]) {
		tud_cdc_rx_drop(itf, amount);
		return;
	}

	/* Reserve some space in the io ring */
	struct io_interface *interface = io_ring_get_device(&devices[itf]->ring);
	void *buffer = io_ring_write_acquire(interface, &amount);
	if (!buffer) {
		tud_cdc_rx_drop(itf, amount);
		return;
	}

	/* Load the buffer and release it */
	amount = tud_cdc_n_read(itf, buffer, amount);
	io_ring_write_release(interface, amount);
}

void tud_cdc_tx_complete_cb(uint8_t itf)
{
	/* Drop if not initialized */
	if (!devices[itf])
		return;

	/* We need to hold the tx lock to prevent a multi-write scenario */
	osMutexAcquire(devices[itf]->tx_lock, osWaitForever);

	size_t room;
	struct io_interface *interface = io_ring_get_device(&devices[itf]->ring);
	while ((room = tud_cdc_n_write_available(itf)) > 0) {

		/* Check for more data */
		size_t avail = SIZE_MAX;
		void *buffer = io_ring_read_acquire(interface, &avail);
		if (avail == 0)
			break;

		/* Limit to amount of room left */
		size_t amount = avail < room ? avail : room;

		/* Send to the usb stack, this will be a copy */
		amount = tud_cdc_n_write(itf, buffer, amount);

		/* Release the space */
		io_ring_read_release(interface, amount);

		/* Start timer if we might have a partial usb tx buffer */
		osStatus_t os_status = osTimerStart(devices[itf]->tx_flush_timer, 10);
		if (os_status != osOK)
			syslog_fatal("could not start flush timer");
	}

	/* All done */
	osMutexRelease(devices[itf]->tx_lock);
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
	/* Ensure the device has been initialized */
	if (!devices[itf]) {
		syslog_warn("device %hhu not initialized, dropping\n", itf);
		return;
	}

	/* Update the line state */
	devices[itf]->line_state.dtr = dtr;
	devices[itf]->line_state.rts = rts;

	/* Let everyone know */
	event_post(EVENT_CDC_LINE_STATE_CHANGED, itf);
}

void tud_cdc_line_coding_cb(uint8_t itf, const cdc_line_coding_t *line_coding)
{
	/* Ensure the device has been initialized */
	if (!devices[itf]) {
		syslog_warn("device %hhu not initialized, dropping\n", itf);
		return;
	}

	/* Decode the parity */
	switch (line_coding->parity) {
		case 0:
			devices[itf]->line_coding.parity = 'n';
			break;
		case 1:
			devices[itf]->line_coding.parity = 'o';
			break;
		case 2:
			devices[itf]->line_coding.parity = 'e';
			break;
		case 3:
			devices[itf]->line_coding.parity = 'm';
			break;
		case 4:
			devices[itf]->line_coding.parity = 's';
			break;
		default:
			syslog_fatal("unknown parity: %hhu\n", line_coding->stop_bits);
	}

	/* Decode the remaining values */
	devices[itf]->line_coding.baud_rate = line_coding->bit_rate;
	devices[itf]->line_coding.data_bits = line_coding->data_bits;
	devices[itf]->line_coding.stops_bits = line_coding->stop_bits == 1 ? 1 : 2;

	/* Let everyone know */
	event_post(EVENT_CDC_LINE_CODING_CHANGED, itf);
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t msec)
{
	syslog_debug("itf: %hhu, duration: %hu\n", itf, msec);
}

static void cdc_serial_flush(void *context)
{
	assert(context != 0);

	struct cdc_serial *serial = context;
	tud_cdc_n_write_flush(serial->channel);

}

static void cdc_serial_host_handler(struct io_interface *interface, enum io_ring_event event, size_t amount, void *context)
{
	assert(interface != 0 && context != 0);

	struct cdc_serial *serial = context;

	if (event == IO_RING_SPACE_AVAIL && amount != 0)
		wait_notify(&serial->space_available, false);

	if (event == IO_RING_DATA_AVAIL && amount != 0)
		wait_notify(&serial->data_available, false);
}

static void cdc_serial_device_handler(struct io_interface *interface, enum io_ring_event event, size_t amount, void *context)
{
	assert(interface != 0 && context != 0);

	struct cdc_serial *serial = context;

	if (event == IO_RING_DATA_AVAIL && amount != 0)
		tud_cdc_tx_complete_cb(serial->channel);
}

struct serial_line_coding cdc_serial_get_line_coding(const struct cdc_serial *serial)
{
	assert(serial != 0);
	return serial->line_coding;
}

struct serial_line_state cdc_serial_get_line_state(const struct cdc_serial *serial)
{
	assert(serial != 0);
	return serial->line_state;
}

unsigned int cdc_serial_get_channel(const struct cdc_serial *serial)
{
	assert(serial != 0);
	return serial->channel;
}

ssize_t cdc_serial_recv(struct cdc_serial *serial, void *buffer, size_t count, unsigned int msecs)
{
	assert(serial != 0 && buffer != 0);

	int status;
	ssize_t amount = 0;
	struct io_interface *interface = io_ring_get_host(&serial->ring);

	/* Only one reciever at time */
	osStatus_t os_status = osMutexAcquire(serial->rx_lock, msecs);
	if (os_status != osOK) {
		if (os_status == osErrorTimeout || (msecs == 0 && os_status == osErrorResource)) {
			errno = ETIMEDOUT;
			return -ETIMEDOUT;
		}
		errno = errno_from_rtos(os_status);
		return -errno;
	}

	/* Block until we read some data */
	while (amount == 0) {

		/* Wait for data */
		if (msecs != osWaitForever) {
			status = wait_event_timeout(&serial->data_available, io_ring_data_available(interface), msecs);
			if (status <= 0) {
				amount = status;
				break;
			}
			msecs -= status;
		} else {
			status = wait_event(&serial->data_available, io_ring_data_available(interface));
			if (status <= 0) {
				amount = status;
				break;
			}
		}

		/* Read all data up to the count */
		amount = io_ring_read(interface, buffer, count);
	}

	/* Release the lock */
	os_status = osMutexRelease(serial->rx_lock);
	if (os_status != osOK) {
		errno = errno_from_rtos(os_status);
		amount = -errno;
	}

	/* Return the amount written */
	return amount;
}

ssize_t cdc_serial_send(struct cdc_serial *serial, const void *buffer, size_t count, unsigned int msecs)
{
	assert(serial != 0 && buffer != 0);

	int status;
	ssize_t amount = 0;
	struct io_interface *interface = io_ring_get_host(&serial->ring);

	/* Only one sender at time */
	osStatus_t os_status = osMutexAcquire(serial->tx_lock, msecs);
	if (os_status != osOK) {
		if (os_status == osErrorTimeout || (msecs == 0 && os_status == osErrorResource)) {
			errno = ETIMEDOUT;
			return -ETIMEDOUT;
		}
		errno = errno_from_rtos(os_status);
		return -errno;
	}

	/* Block until some data sent */
	while (amount == 0) {

		/* Wait for space */
		if (msecs != osWaitForever) {
			status = wait_event_timeout(&serial->space_available, io_ring_space_available(interface), msecs);
			if (status < 0) {
				amount = status;
				break;
			}
			msecs -= status;
		} else {
			status = wait_event(&serial->space_available, io_ring_space_available(interface));
			if (status < 0) {
				amount = status;
				break;
			}
		}

		/* Send all data up to the count */
		amount += io_ring_write(interface, buffer + amount, count - amount);
	}

	/* Release the lock */
	os_status = osMutexRelease(serial->tx_lock);
	if (os_status != osOK) {
		errno = errno_from_rtos(os_status);
		amount = -errno;
	}

	/* Return the amount written */
	return amount;
}

int cdc_serial_ini(struct cdc_serial *serial, unsigned int channel)
{
	assert(serial != 0 && channel < CFG_TUD_CDC);

	char rtos_name[RTOS_NAME_SIZE] = { [0 ... RTOS_NAME_SIZE - 1] = 0 };

	/* Basic initialization */
	memset(serial, 0, sizeof(*serial));
	serial->channel = channel;

	/* Initialize the io ring */
	int status = io_ring_ini(&serial->ring, 0, CFG_TUD_CDC_EP_BUFSIZE * 2);
	if (status < 0)
		return status;

	/* Bind the host io ring handlers */
	io_ring_set_callback(io_ring_get_host(&serial->ring), cdc_serial_host_handler, serial);
	io_ring_set_callback(io_ring_get_device(&serial->ring), cdc_serial_device_handler, serial);

	/* Initialize the wait queues */
	wait_queue_ini(&serial->data_available);
	wait_queue_ini(&serial->space_available);

	/* Create the rx and tx locks */
	snprintf(rtos_name, RTOS_NAME_SIZE - 1, "acm%d-rx", channel);
	osMutexAttr_t mutex_attr = { .name = rtos_name, .attr_bits = osMutexPrioInherit | osMutexRecursive };
	serial->rx_lock = osMutexNew(&mutex_attr);
	if (!serial->rx_lock) {
		errno = ERTOS;
		status = -ERTOS;
		goto error_release_ring;
	}
	snprintf(rtos_name, RTOS_NAME_SIZE - 1, "acm%d-tx", channel);
	serial->tx_lock = osMutexNew(&mutex_attr);
	if (!serial->tx_lock) {
		errno = ERTOS;
		status = -ERTOS;
		goto error_delete_rx_lock;
	}

	/* Create the channel flush timer */
	snprintf(rtos_name, RTOS_NAME_SIZE - 1, "acm%d", channel);
	osTimerAttr_t timer_attr = { .name = rtos_name };
	serial->tx_flush_timer = osTimerNew(cdc_serial_flush, osTimerOnce, serial, &timer_attr);
	if (!serial->tx_flush_timer) {
		errno = ERTOS;
		status = -ERTOS;
		goto error_delete_tx_lock;
	}

	/* Hold a reference to the usb-device */
	serial->usbd_fd = open("USBD", O_RDWR);
	if (serial->usbd_fd < 0) {
		io_ring_fini(&serial->ring);
		status = -errno;
		goto error_delete_timer;
	}

	/* All good by this point */
	return 0;

error_delete_timer:
	osTimerDelete(serial->tx_flush_timer);

error_delete_tx_lock:
	osMutexDelete(serial->tx_lock);

error_delete_rx_lock:
	osMutexDelete(serial->rx_lock);

error_release_ring:
	io_ring_fini(&serial->ring);

	return status;
}

void cdc_serial_fini(struct cdc_serial *serial)
{
	assert(serial != 0);

	/* Release the usb device handle */
	close(serial->usbd_fd);

	/* Delete the flush timer */
	osTimerDelete(serial->tx_flush_timer);

	/* Delete the locks */
	osMutexDelete(serial->tx_lock);
	osMutexDelete(serial->rx_lock);

	/* Release the wait queues */
	wait_queue_fini(&serial->space_available);
	wait_queue_fini(&serial->data_available);

	/* Release the io rings */
	io_ring_fini(&serial->ring);

	/* At last, the device */
	free(serial);
}

struct cdc_serial *cdc_serial_create(unsigned int channel)
{
	/* Get some memory */
	struct cdc_serial *serial = malloc(sizeof(*serial));
	if (!serial)
		return 0;

	/* Forward */
	int status = cdc_serial_ini(serial, channel);
	if (status < 0) {
		free(serial);
		serial = 0;
	}

	return serial;
}

void cdc_serial_destroy(struct cdc_serial *serial)
{
	assert(serial != 0);

	/* Forward */
	cdc_serial_fini(serial);

	/* Clean up */
	free(serial);
}

static void cdc_serial_device_release(struct ref *ref)
{
	assert(ref != 0);

	struct cdc_serial *serial = container_of(ref, struct cdc_serial, ref);

	/* Clear the device entry */
	devices[serial->channel] = 0;
	device_init_flags[serial->channel] = osOnceFlagsInit;

	/* We are the last reference */
	cdc_serial_destroy(serial);
}

static void cdc_serial_device_init(osOnceFlagId_t flag_id, void *context)
{
	unsigned int channel = (unsigned int)context;

	/* Try to create the device */
	devices[channel] = cdc_serial_create(channel);
	if (!devices[channel])
		syslog_fatal("failed to create ACM%u: %d\n", channel, errno);

	/* Save the channel */
	devices[channel]->channel = channel;

	/* Initialize the reference count */
	ref_ini(&devices[channel]->ref, cdc_serial_device_release, 0);
}

static struct cdc_serial *cdc_serial_get(unsigned int channel)
{
	assert(channel < CFG_TUD_CDC);

	/* Setup the matching cdc serial device */
	osCallOnce(&device_init_flags[channel], cdc_serial_device_init, (void *)channel);

	/* Update the count */
	ref_up(&devices[channel]->ref);

	/* Add return it */
	return devices[channel];
}

static void cdc_serial_put(struct cdc_serial *serial)
{
	assert(serial != 0);

	ref_down(&serial->ref);
}

static int cdc_serial_posix_close(int fd)
{
	assert(fd > 0);

	struct cdc_serial_file *file = container_of((struct posix_ops *)fd, struct cdc_serial_file, ops);

	/* Release reference to the device */
	cdc_serial_put(file->device);

	/* Release the uart serial file */
	free(file);

	/* Always good */
	return 0;
}

static ssize_t cdc_serial_posix_read(int fd, void *buf, size_t count)
{
	return cdc_serial_recv(cdc_serial_from_fd(fd), buf, count, osWaitForever);
}

static ssize_t cdc_serial_posix_write(int fd, const void *buf, size_t count)
{
	return cdc_serial_send(cdc_serial_from_fd(fd), buf, count, osWaitForever);
}

static off_t cdc_serial_posix_lseek(int fd, off_t offset, int whence)
{
	errno = ENOTSUP;
	return (off_t)-1;
}

static int cdc_serial_open(const char *path, int flags)
{
	/* Check the path */
	if (path == 0 || strlen(path) < strlen("ACM")) {
		errno = EINVAL;
		return -1;
	}

	/* Extract the channel index */
	errno = 0;
	char *end;
	unsigned int channel = strtoul(path + 3, &end, 0);
	if (errno != 0 || end == path + 3 || channel >= CFG_TUD_CDC) {
		errno = ENODEV;
		return -1;
	}

	/* Allocate the uart serial channel */
	struct cdc_serial_file *file = calloc(1, sizeof(struct cdc_serial_file));
	if (!file) {
		errno = ENOMEM;
		return -ENOMEM;
	}

	/* Get the underlaying uart device */
	file->device = cdc_serial_get(channel);

	/* Setup the posix interface */
	file->ops.close = cdc_serial_posix_close;
	file->ops.read = cdc_serial_posix_read;
	file->ops.write = cdc_serial_posix_write;
	file->ops.lseek = cdc_serial_posix_lseek;

	/* Return the ops as a integer */
	return (intptr_t)&file->ops;
}

static __constructor_priority(DEVICES_PRIORITY) void cdc_serial_posix_ini(void)
{
	int status = posix_register("ACM", cdc_serial_open);
	if (status != 0)
		syslog_fatal("failed to register posix device: %d\n", errno);
}

static __destructor_priority(DEVICES_PRIORITY) void cdc_serial_posix_fini(void)
{
	posix_unregister("ACM");
}
