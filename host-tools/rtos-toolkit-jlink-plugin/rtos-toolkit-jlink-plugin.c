#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PLUGIN_VERSION 101

#define RTOS_SYMBOL_SCHEDULER 0UL
#define RTOS_SYMBOL_SCHEDULER_FRAME_LAYOUT 1UL
#define RTOS_SYMBOL_SCHEDULER_LAYOUT 2UL
#define RTOS_SYMBOL_TASK_LAYOUT 3UL

#define RTOS_SCHEDULER_MARKER 0x13700731UL
#define RTOS_TASK_MARKER 0x137aa731UL

#define RTOS_TASK_NAME_LEN 32

#define RTOS_PLUGIN_BUF_SIZE_THREAD_DISPLAY 256

#define JLINK_CORE_CORTEX_M0 0x060000FFUL
#define JLINK_CORE_CORTEX_M3 0x030000FFUL
#define JLINK_CORE_CORTEX_M3_R1P0 0x03000010UL
#define JLINK_CORE_CORTEX_M3_R1P1 0x03000011UL
#define JLINK_CORE_CORTEX_M3_R2P0 0x03000020UL
#define JLINK_CORE_CORTEX_M4 0x0E0000FFUL
#define JLINK_CORE_CORTEX_M7 0x0E0100FFUL

#define RTOS_REG_CORTEX_M_R0 0UL
#define RTOS_REG_CORTEX_M_R1 1UL
#define RTOS_REG_CORTEX_M_R2 2UL
#define RTOS_REG_CORTEX_M_R3 3UL
#define RTOS_REG_CORTEX_M_R4 4UL
#define RTOS_REG_CORTEX_M_R5 5UL
#define RTOS_REG_CORTEX_M_R6 6UL
#define RTOS_REG_CORTEX_M_R7 7UL
#define RTOS_REG_CORTEX_M_R8 8UL
#define RTOS_REG_CORTEX_M_R9 9UL
#define RTOS_REG_CORTEX_M_R10 10UL
#define RTOS_REG_CORTEX_M_R11 11UL
#define RTOS_REG_CORTEX_M_R12 12UL
#define RTOS_REG_CORTEX_M_SP 13UL
#define RTOS_REG_CORTEX_M_LR 14UL
#define RTOS_REG_CORTEX_M_PC 15UL
#define RTOS_REG_CORTEX_M_XPSR 16UL
#define RTOS_REG_CORTEX_M_MSP 17UL
#define RTOS_REG_CORTEX_M_PSP 18UL
#define RTOS_REG_CORTEX_M_PRIMASK 19UL
#define RTOS_REG_CORTEX_M_BASEPRI 20UL
#define RTOS_REG_CORTEX_M_FAULTMASK 21UL
#define RTOS_REG_CORTEX_M_CONTROL 22UL
#define RTOS_REG_CORTEX_M_FPSCR 23UL
#define RTOS_REG_CORTEX_M_S0 24UL
#define RTOS_REG_CORTEX_M_S1 25UL
#define RTOS_REG_CORTEX_M_S2 26UL
#define RTOS_REG_CORTEX_M_S3 27UL
#define RTOS_REG_CORTEX_M_S4 28UL
#define RTOS_REG_CORTEX_M_S5 29UL
#define RTOS_REG_CORTEX_M_S6 30UL
#define RTOS_REG_CORTEX_M_S7 31UL
#define RTOS_REG_CORTEX_M_S8 32UL
#define RTOS_REG_CORTEX_M_S9 33UL
#define RTOS_REG_CORTEX_M_S10 34UL
#define RTOS_REG_CORTEX_M_S11 35UL
#define RTOS_REG_CORTEX_M_S12 36UL
#define RTOS_REG_CORTEX_M_S13 37UL
#define RTOS_REG_CORTEX_M_S14 38UL
#define RTOS_REG_CORTEX_M_S15 39UL
#define RTOS_REG_CORTEX_M_S16 40UL
#define RTOS_REG_CORTEX_M_S17 41UL
#define RTOS_REG_CORTEX_M_S18 42UL
#define RTOS_REG_CORTEX_M_S19 43UL
#define RTOS_REG_CORTEX_M_S20 44UL
#define RTOS_REG_CORTEX_M_S21 45UL
#define RTOS_REG_CORTEX_M_S22 46UL
#define RTOS_REG_CORTEX_M_S23 47UL
#define RTOS_REG_CORTEX_M_S24 48UL
#define RTOS_REG_CORTEX_M_S25 49UL
#define RTOS_REG_CORTEX_M_S26 50UL
#define RTOS_REG_CORTEX_M_S27 51UL
#define RTOS_REG_CORTEX_M_S28 52UL
#define RTOS_REG_CORTEX_M_S29 53UL
#define RTOS_REG_CORTEX_M_S30 54UL
#define RTOS_REG_CORTEX_M_S31 55UL
#define RTOS_REG_CORTEX_M_D0 56UL
#define RTOS_REG_CORTEX_M_D1 57UL
#define RTOS_REG_CORTEX_M_D2 58UL
#define RTOS_REG_CORTEX_M_D3 59UL
#define RTOS_REG_CORTEX_M_D4 60UL
#define RTOS_REG_CORTEX_M_D5 61UL
#define RTOS_REG_CORTEX_M_D6 62UL
#define RTOS_REG_CORTEX_M_D7 63UL
#define RTOS_REG_CORTEX_M_D8 64UL
#define RTOS_REG_CORTEX_M_D9 65UL
#define RTOS_REG_CORTEX_M_D10 66UL
#define RTOS_REG_CORTEX_M_D11 67UL
#define RTOS_REG_CORTEX_M_D12 68UL
#define RTOS_REG_CORTEX_M_D13 69UL
#define RTOS_REG_CORTEX_M_D14 70UL
#define RTOS_REG_CORTEX_M_D15 71UL
#define RTOS_REG_CORTEX_M_NUMREGS 72UL

struct rtos_symbols
{
	const char *name;
	int optional;
	uint32_t address;
};

struct gdb_api
{
	/* API version v1.0 and higher */
	void (*free)(void *ptr);
	void* (*alloc)(unsigned size);
	void* (*realloc)(void *ptr, unsigned int size);
	void (*log_output)(const char *fmt, ...);
	void (*debug_output)(const char *fmt, ...);
	void (*warn_output)(const char *fmt, ...);
	void (*error_output)(const char *fmt, ...);
	int (*read)(uint32_t addr, void *buffer, unsigned int count);
	char (*read_uint8)(uint32_t addr, uint8_t *value);
	char (*read_uint16)(uint32_t addr, uint16_t *value);
	char (*read_uint32)(uint32_t addr, uint32_t *value);
	int (*write)(uint32_t addr, const void* buffer, unsigned int count);
	void (*write_uint8)(uint32_t addr, uint8_t value);
	void (*write_uint16)(uint32_t addr, uint16_t value);
	void (*write_uint32)(uint32_t addr, uint32_t value);
	uint32_t (*load_16)(const uint8_t *buffer);
	uint32_t (*load_24)(const uint8_t *buffer);
	uint32_t (*load_32)(const uint8_t *buffer);

	/* API version v1.1 and higher */
	uint32_t (*read_reg)(uint32_t index);
	void (*write_reg)(uint32_t index, uint32_t value);

	/* End marker */
	void *marker;
};

enum rtos_task_state
{
	TASK_OTHER = -1,
	TASK_TERMINATED = 0,
	TASK_RUNNING = 1,
	TASK_READY = 2,
	TASK_BLOCKED = 3,
	TASK_SLEEPING = 4,
	TASK_SUSPENDED = 5,
	TASK_UNKNOWN = 6,
	TASK_RESERVED = 0x7fffffff,
};

struct rtostoolkit_task
{
	uint32_t task_id;
	int32_t state;
	uint32_t current_priority;
	char name[RTOS_TASK_NAME_LEN];
	size_t stack_size;
	uint32_t flags;
	uint32_t psp;
};

struct field_desc
{
	const char *name;
	uint32_t offset;
	uint32_t size;
};

struct layout {
	size_t size;
	size_t count;
	struct field_desc desc[];
};

int RTOS_Init(const struct gdb_api *gdb_api, uint32_t core);
uint32_t RTOS_GetVersion(void);
struct rtos_symbols* RTOS_GetSymbols(void);
uint32_t RTOS_GetNumThreads(void);
uint32_t RTOS_GetCurrentThreadId(void);
uint32_t RTOS_GetThreadId(uint32_t n);
int RTOS_GetThreadDisplay(char *buffer, uint32_t thread_id);
int RTOS_GetThreadReg(char *data, uint32_t index, uint32_t thread_id);
int RTOS_GetThreadRegList(char *data, uint32_t thread_id);
int RTOS_SetThreadReg(char *data, uint32_t index, uint32_t thread_id);
int RTOS_SetThreadRegList(char *data, uint32_t thread_id);
int RTOS_UpdateThreads(void);

const struct gdb_api *gdb_api = 0;
struct layout *scheduler_layout = 0;
struct layout *scheduler_frame_layout = 0;
struct layout *task_layout = 0;

uint32_t num_tasks = 0;
size_t tasks_size = 8;
struct rtostoolkit_task *tasks = 0;
uint32_t current_task = 0;
uint32_t register_stack_offsets[RTOS_REG_CORTEX_M_NUMREGS] = { 0 };
uint32_t stack_frame_task_id = 0;
uint8_t *stack_frame = 0;

static struct rtos_symbols symbols[] =
{
	{ "scheduler", 0, 0 },
	{ "scheduler_frame_layout", 0, 0 },
	{ "scheduler_layout", 0, 0 },
	{ "task_layout", 0, 0 },
	{ 0, 0, 0 },
};

static const char *rtos_task_state_to_str(enum rtos_task_state state)
{
	switch (state) {
		case TASK_OTHER:
			return "OTHER";
		case TASK_TERMINATED:
			return "TERMINATED";
		case TASK_SUSPENDED:
			return "SUSPENDED";
		case TASK_RUNNING:
			return "RUNNING";
		case TASK_READY:
			return "READY";
		case TASK_BLOCKED:
			return "BLOCKED";
		case TASK_SLEEPING:
			return "SLEEPING";
		default:
			return "UNKNOWN";
	}
}

static int compare_task_id(const void *first, const void *second)
{
	const struct rtostoolkit_task *first_task = first;
	const struct rtostoolkit_task *second_task = second;

	return first_task->task_id - second_task->task_id;
}

static int compare_field_desc_name(const void *first, const void *second)
{
	const struct field_desc *first_desc = first;
	const struct field_desc *second_desc = second;

	return strcmp(first_desc->name, second_desc->name);
}

static size_t lookup_field_offset(const struct layout *layout, const char *name)
{
	struct field_desc key = { .name = name };
	const struct field_desc *desc = bsearch(&key, layout->desc, layout->count, sizeof(*layout->desc), compare_field_desc_name);

	return desc ? desc->offset : SIZE_MAX;
}

static struct rtostoolkit_task *lookup_task(uint32_t task_id)
{
	struct rtostoolkit_task key = { .task_id = task_id };
	return bsearch(&key, tasks, num_tasks, sizeof(struct rtostoolkit_task), compare_task_id);
}

static char *load_str(uint32_t address)
{
	/* Guess at string length */
	size_t size = 0;
	char *str = gdb_api->alloc(size);

	do {

		/* Get some space */
		size += 32;
		str = gdb_api->realloc(str, size);
		if (!str) {
			gdb_api->free(str);
			return 0;
		}

		/* Read 32 bytes and hope we found the null and do not fail because we ran off the end of readable memory */
		if (gdb_api->read(address, str, 32) < 0) {
			gdb_api->free(str);
			return 0;
		}

	} while (strnlen(str, size) == size);

	return str;
}

static struct layout *load_layout(uint32_t address)
{
	uint32_t field_name_addr;
	size_t size = 0;
	struct layout *layout = 0;
	int count = 0;

	do {

		/* Get some space if needed */
		if (count * sizeof(struct field_desc) + sizeof(struct layout) >= size) {
			size += (count + 16) * sizeof(struct field_desc) + sizeof(struct layout);
			struct layout *new_layout = gdb_api->realloc(layout, size);
			if (!new_layout) {
				gdb_api->error_output("RTOS-TOOLKIT: failed to allocate %u bytes for layout\n", size);
				goto error_release_layout;
			}
			layout = new_layout;
		}

		/* Field name address */
		if (gdb_api->read_uint32(address, &field_name_addr) < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read field name addr at 0x%x\n", address);
			goto error_release_layout;
		}
		address += 4;

		/* If we are not at the end of the list */
		if (field_name_addr) {

			/* Read the string */
			layout->desc[count].name = load_str(field_name_addr);
			if (!layout->desc[count].name) {
				gdb_api->error_output("RTOS-TOOLKIT: failed to load field name at 0x%x\n", field_name_addr);
				goto error_release_layout;
			}

			/* Read the field offset */
			if (gdb_api->read_uint32(address, &layout->desc[count].offset) < 0) {
				gdb_api->error_output("RTOS-TOOLKIT: failed to read field offset at 0x%x\n", address);
				goto error_release_layout;
			}
			address += 4;

			/* Read the field size */
			if (gdb_api->read_uint32(address, &layout->desc[count].size) < 0) {
				gdb_api->error_output("RTOS-TOOLKIT: failed to read field size at 0x%x\n", address);
				goto error_release_layout;
			}
			address += 4;
		}

		/* Next descriptor */
		++count;

	} while (field_name_addr);

	/* Save the number of fields */
	layout->count = count - 1;

	/* Read the struct size */
	address += 4;
	uint32_t struct_size;
	if (gdb_api->read_uint32(address, &struct_size) < 0) {
		gdb_api->error_output("RTOS-TOOLKIT: failed to read struct size at 0x%x\n", address);
		goto error_release_layout;
	}
	layout->size = struct_size;

	/* Sort the layout by name */
	qsort(layout->desc, layout->count, sizeof(struct field_desc), compare_field_desc_name);

	/* All good */
	return layout;

error_release_layout:
	gdb_api->free(layout);

	return 0;
}

static __attribute__((unused)) void dump_layout(const char *name, const struct layout *layout)
{
	gdb_api->log_output("%s: %p size: %u fields: %u\n", name, layout, layout->size, layout->count);
	for (int i = 0; i < layout->count; ++i) {
		gdb_api->log_output("  name: %p %s\n", layout->desc[i].name, layout->desc[i].name);
		gdb_api->log_output("    offset: %u\n", layout->desc[i].offset);
		gdb_api->log_output("    size: %u\n", layout->desc[i].size);
	}
}

int RTOS_Init(const struct gdb_api *api, uint32_t core)
{
	/* Check the core type */
	if (core != JLINK_CORE_CORTEX_M0 && core != JLINK_CORE_CORTEX_M4 && core != JLINK_CORE_CORTEX_M7) {
		api->error_output("RTOS-TOOLKIT: unsupported core type: 0x%x\n", core);
		return 0;
	}

	/* Save the api */
	gdb_api = api;

	/* Allocate the initialize task buffer */
	tasks = api->alloc(sizeof(struct rtostoolkit_task) * tasks_size);
	if (!tasks) {
		api->error_output("RTOS-TOOLKIT: could not allocate initial task buffer\n");
		return 0;
	}

	/* Log the initialization */
	api->log_output("RTOS-TOOLKIT: plugin initialized with core 0x%x\n", core);

	return 1;
}

uint32_t RTOS_GetVersion(void)
{
	return PLUGIN_VERSION;
}

struct rtos_symbols* RTOS_GetSymbols(void)
{
	return symbols;
}

uint32_t RTOS_GetNumThreads(void)
{
	return num_tasks;
}

uint32_t RTOS_GetCurrentThreadId(void)
{
	return current_task;
}

uint32_t RTOS_GetThreadId(uint32_t n)
{
	return tasks[n].task_id;
}

int RTOS_GetThreadDisplay(char *buffer, uint32_t thread_id)
{
	struct rtostoolkit_task *task = lookup_task(thread_id);
	if (!task) {
		gdb_api->error_output("RTOS_TOOLKIT: could not find task id: 0x%x\n", thread_id);
		return 0;
	}

	snprintf(buffer, RTOS_PLUGIN_BUF_SIZE_THREAD_DISPLAY, "%s@%x [S:%s(%d)] [P:%u]", task->name, task->task_id, rtos_task_state_to_str(task->state), task->state, task->current_priority);
	buffer[RTOS_PLUGIN_BUF_SIZE_THREAD_DISPLAY - 1] = 0;

	return strlen(buffer);
}

int RTOS_GetThreadReg(char *data, uint32_t index, uint32_t thread_id)
{
	/* Scheduler not started or there is not current task */
	if (thread_id == 0)
		return -1;

	/* Look up the task */
	struct rtostoolkit_task *task = lookup_task(thread_id);
	if (!task) {
		gdb_api->error_output("RTOS-TOOLKIT: could not find task 0x%x\n", thread_id);
		return -1;
	}

	/* The current task register must be read from the CPU directly */
	if (task->task_id == current_task)
		return -1;

	/* Some registers are always read from the CPU */
	if (index <= RTOS_REG_CORTEX_M_CONTROL)
		return -1;

	/* Do we need to load the stack frame? */
	if (stack_frame_task_id != task->task_id) {
		int status = gdb_api->read(task->psp, (char *)stack_frame, scheduler_frame_layout->size);
		if (status != scheduler_frame_layout->size) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the task stack frame(%u bytes): %u\n", scheduler_frame_layout->size, status);
			return -EIO;
		}
		stack_frame_task_id = task->task_id;
	}

	/* Get the value */
	uint32_t value = 0;
	if (index == RTOS_REG_CORTEX_M_SP)
		value = task->psp + scheduler_frame_layout->size;
	else if (register_stack_offsets[index] != UINT32_MAX)
		value = *((uint32_t *)(stack_frame + register_stack_offsets[index]));

	/* Generate the hex string, very strange format, null separated hex string bytes, such bullshit */
	uint8_t *buffer = (uint8_t *)&value;
	data += snprintf(data, 3, "%02x", buffer[0]);
	data += snprintf(data, 3, "%02x", buffer[1]);
	data += snprintf(data, 3, "%02x", buffer[2]);
	data += snprintf(data, 3, "%02x", buffer[3]);

	/* All done */
	return 0;
}

int RTOS_GetThreadRegList(char *data, uint32_t thread_id)
{
	/* Scheduler not started or there is not current task */
	if (thread_id == 0)
		return -1;

	/* Look up the task */
	struct rtostoolkit_task *task = lookup_task(thread_id);
	if (!task) {
		gdb_api->error_output("RTOS-TOOLKIT: could not find task 0x%x\n", thread_id);
		return -1;
	}

	/* The current task register must be read from the CPU directly */
	if (task->task_id == current_task)
		return -1;

	/* Do we need to load the stack frame? */
	if (stack_frame_task_id != task->task_id) {
		int status = gdb_api->read(task->psp, (char *)stack_frame, scheduler_frame_layout->size);
		if (status != scheduler_frame_layout->size) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the task stack frame(%u bytes): %u\n", scheduler_frame_layout->size, status);
			return -EIO;
		}
		stack_frame_task_id = task->task_id;
	}

	/* Loop through the general registers */
	for (int i = RTOS_REG_CORTEX_M_R0; i <= RTOS_REG_CORTEX_M_XPSR; ++i) {

		/* Get the value */
		uint32_t value = 0;
		if (i == RTOS_REG_CORTEX_M_SP)
			value = task->psp + scheduler_frame_layout->size;
		else if (register_stack_offsets[i] != UINT32_MAX)
			value = *((uint32_t *)(stack_frame + register_stack_offsets[i]));

		/* Write the null terminate hex bytes */
		uint8_t *buffer = (uint8_t *)&value;
		data += snprintf(data, 3, "%02x", buffer[0]);
		data += snprintf(data, 3, "%02x", buffer[1]);
		data += snprintf(data, 3, "%02x", buffer[2]);
		data += snprintf(data, 3, "%02x", buffer[3]);
	}

	return 0;
}

int RTOS_SetThreadReg(char *data, uint32_t index, uint32_t thread_id)
{
	/* Scheduler not started or there is not current task */
	if (thread_id == 0)
		return -1;

	/* Look up the task */
	struct rtostoolkit_task *task = lookup_task(thread_id);
	if (!task) {
		gdb_api->error_output("RTOS-TOOLKIT: could not find task 0x%x\n", thread_id);
		return -1;
	}

	/* The current task register must be read from the CPU directly */
	if (task->task_id == current_task)
		return -1;

	/* Not really supported */
	return 0;
}

int RTOS_SetThreadRegList(char *data, uint32_t thread_id)
{
	/* Scheduler not started or there is not current task */
	if (thread_id == 0)
		return -1;

	/* Look up the task */
	struct rtostoolkit_task *task = lookup_task(thread_id);
	if (!task) {
		gdb_api->error_output("RTOS-TOOLKIT: could not find task 0x%x\n", thread_id);
		return -1;
	}

	/* The current task register must be read from the CPU directly */
	if (task->task_id == current_task)
		return -1;

	/* Not really supported */
	return 0;
}

int RTOS_UpdateThreads(void)
{
	uint32_t scheduler_ptr;
	int status;

	/* All way initialize num tasks; */
	num_tasks = 0;
	stack_frame_task_id = UINT32_MAX;

	/* Ensure we have a scheduler layout */
	if (!scheduler_layout) {
		scheduler_layout = load_layout(symbols[RTOS_SYMBOL_SCHEDULER_LAYOUT].address);
		if (!scheduler_layout) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to load scheduler layout\n");
			return -ENOTSUP;
		}
	}

	/* And the matching scheduler frame layout */
	if (!scheduler_frame_layout) {
		scheduler_frame_layout = load_layout(symbols[RTOS_SYMBOL_SCHEDULER_FRAME_LAYOUT].address);
		if (!scheduler_frame_layout) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to load scheduler frame layout\n");
			return -ENOTSUP;
		}

		/* Allocate the stack frame buffer */
		stack_frame = gdb_api->alloc(scheduler_frame_layout->size);
		if (!stack_frame) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to allocate %u bytes for stack frame\n", scheduler_frame_layout->size);
			return -ENOMEM;
		}

		/* Initialize the register stack offsets */
		register_stack_offsets[RTOS_REG_CORTEX_M_R0] = lookup_field_offset(scheduler_frame_layout, "r0");
		register_stack_offsets[RTOS_REG_CORTEX_M_R1] = lookup_field_offset(scheduler_frame_layout, "r1");
		register_stack_offsets[RTOS_REG_CORTEX_M_R2] = lookup_field_offset(scheduler_frame_layout, "r2");
		register_stack_offsets[RTOS_REG_CORTEX_M_R3] = lookup_field_offset(scheduler_frame_layout, "r3");
		register_stack_offsets[RTOS_REG_CORTEX_M_R4] = lookup_field_offset(scheduler_frame_layout, "r4");
		register_stack_offsets[RTOS_REG_CORTEX_M_R5] = lookup_field_offset(scheduler_frame_layout, "r5");
		register_stack_offsets[RTOS_REG_CORTEX_M_R6] = lookup_field_offset(scheduler_frame_layout, "r6");
		register_stack_offsets[RTOS_REG_CORTEX_M_R7] = lookup_field_offset(scheduler_frame_layout, "r7");
		register_stack_offsets[RTOS_REG_CORTEX_M_R8] = lookup_field_offset(scheduler_frame_layout, "r8");
		register_stack_offsets[RTOS_REG_CORTEX_M_R9] = lookup_field_offset(scheduler_frame_layout, "r9");
		register_stack_offsets[RTOS_REG_CORTEX_M_R10] = lookup_field_offset(scheduler_frame_layout, "r10");
		register_stack_offsets[RTOS_REG_CORTEX_M_R11] = lookup_field_offset(scheduler_frame_layout, "r11");
		register_stack_offsets[RTOS_REG_CORTEX_M_R12] = lookup_field_offset(scheduler_frame_layout, "r12");
		register_stack_offsets[RTOS_REG_CORTEX_M_SP] = UINT32_MAX,
		register_stack_offsets[RTOS_REG_CORTEX_M_LR] = lookup_field_offset(scheduler_frame_layout, "lr");
		register_stack_offsets[RTOS_REG_CORTEX_M_PC] = lookup_field_offset(scheduler_frame_layout, "pc");
		register_stack_offsets[RTOS_REG_CORTEX_M_XPSR] = lookup_field_offset(scheduler_frame_layout, "psr");
		register_stack_offsets[RTOS_REG_CORTEX_M_MSP] = UINT32_MAX;
		register_stack_offsets[RTOS_REG_CORTEX_M_PSP] = UINT32_MAX;
		register_stack_offsets[RTOS_REG_CORTEX_M_PRIMASK] = UINT32_MAX;
		register_stack_offsets[RTOS_REG_CORTEX_M_BASEPRI] = lookup_field_offset(scheduler_frame_layout, "basepri");
		register_stack_offsets[RTOS_REG_CORTEX_M_FAULTMASK] = UINT32_MAX;
		register_stack_offsets[RTOS_REG_CORTEX_M_CONTROL] = lookup_field_offset(scheduler_frame_layout, "control");
		register_stack_offsets[RTOS_REG_CORTEX_M_FPSCR] = lookup_field_offset(scheduler_frame_layout, "fpscr");

		register_stack_offsets[RTOS_REG_CORTEX_M_S0] = lookup_field_offset(scheduler_frame_layout, "s0");
		register_stack_offsets[RTOS_REG_CORTEX_M_S1] = lookup_field_offset(scheduler_frame_layout, "s1");
		register_stack_offsets[RTOS_REG_CORTEX_M_S2] = lookup_field_offset(scheduler_frame_layout, "s2");
		register_stack_offsets[RTOS_REG_CORTEX_M_S3] = lookup_field_offset(scheduler_frame_layout, "s3");
		register_stack_offsets[RTOS_REG_CORTEX_M_S4] = lookup_field_offset(scheduler_frame_layout, "s4");
		register_stack_offsets[RTOS_REG_CORTEX_M_S5] = lookup_field_offset(scheduler_frame_layout, "s5");
		register_stack_offsets[RTOS_REG_CORTEX_M_S6] = lookup_field_offset(scheduler_frame_layout, "s6");
		register_stack_offsets[RTOS_REG_CORTEX_M_S7] = lookup_field_offset(scheduler_frame_layout, "s7");
		register_stack_offsets[RTOS_REG_CORTEX_M_S8] = lookup_field_offset(scheduler_frame_layout, "s8");
		register_stack_offsets[RTOS_REG_CORTEX_M_S9] = lookup_field_offset(scheduler_frame_layout, "s9");
		register_stack_offsets[RTOS_REG_CORTEX_M_S10] = lookup_field_offset(scheduler_frame_layout, "s10");
		register_stack_offsets[RTOS_REG_CORTEX_M_S11] = lookup_field_offset(scheduler_frame_layout, "s11");
		register_stack_offsets[RTOS_REG_CORTEX_M_S12] = lookup_field_offset(scheduler_frame_layout, "s12");
		register_stack_offsets[RTOS_REG_CORTEX_M_S13] = lookup_field_offset(scheduler_frame_layout, "s13");
		register_stack_offsets[RTOS_REG_CORTEX_M_S14] = lookup_field_offset(scheduler_frame_layout, "s14");
		register_stack_offsets[RTOS_REG_CORTEX_M_S15] = lookup_field_offset(scheduler_frame_layout, "s15");
		register_stack_offsets[RTOS_REG_CORTEX_M_S16] = lookup_field_offset(scheduler_frame_layout, "s16");
		register_stack_offsets[RTOS_REG_CORTEX_M_S17] = lookup_field_offset(scheduler_frame_layout, "s17");
		register_stack_offsets[RTOS_REG_CORTEX_M_S18] = lookup_field_offset(scheduler_frame_layout, "s18");
		register_stack_offsets[RTOS_REG_CORTEX_M_S19] = lookup_field_offset(scheduler_frame_layout, "s19");
		register_stack_offsets[RTOS_REG_CORTEX_M_S20] = lookup_field_offset(scheduler_frame_layout, "s20");
		register_stack_offsets[RTOS_REG_CORTEX_M_S21] = lookup_field_offset(scheduler_frame_layout, "s21");
		register_stack_offsets[RTOS_REG_CORTEX_M_S22] = lookup_field_offset(scheduler_frame_layout, "s22");
		register_stack_offsets[RTOS_REG_CORTEX_M_S23] = lookup_field_offset(scheduler_frame_layout, "s23");
		register_stack_offsets[RTOS_REG_CORTEX_M_S24] = lookup_field_offset(scheduler_frame_layout, "s24");
		register_stack_offsets[RTOS_REG_CORTEX_M_S25] = lookup_field_offset(scheduler_frame_layout, "s25");
		register_stack_offsets[RTOS_REG_CORTEX_M_S26] = lookup_field_offset(scheduler_frame_layout, "s26");
		register_stack_offsets[RTOS_REG_CORTEX_M_S27] = lookup_field_offset(scheduler_frame_layout, "s27");
		register_stack_offsets[RTOS_REG_CORTEX_M_S28] = lookup_field_offset(scheduler_frame_layout, "s28");
		register_stack_offsets[RTOS_REG_CORTEX_M_S29] = lookup_field_offset(scheduler_frame_layout, "s29");
		register_stack_offsets[RTOS_REG_CORTEX_M_S30] = lookup_field_offset(scheduler_frame_layout, "s30");
		register_stack_offsets[RTOS_REG_CORTEX_M_S31] = lookup_field_offset(scheduler_frame_layout, "s31");

		register_stack_offsets[RTOS_REG_CORTEX_M_D0] = lookup_field_offset(scheduler_frame_layout, "s0");
		register_stack_offsets[RTOS_REG_CORTEX_M_D1] = lookup_field_offset(scheduler_frame_layout, "s2");
		register_stack_offsets[RTOS_REG_CORTEX_M_D2] = lookup_field_offset(scheduler_frame_layout, "s4");
		register_stack_offsets[RTOS_REG_CORTEX_M_D3] = lookup_field_offset(scheduler_frame_layout, "s6");
		register_stack_offsets[RTOS_REG_CORTEX_M_D4] = lookup_field_offset(scheduler_frame_layout, "s8");
		register_stack_offsets[RTOS_REG_CORTEX_M_D5] = lookup_field_offset(scheduler_frame_layout, "s10");
		register_stack_offsets[RTOS_REG_CORTEX_M_D6] = lookup_field_offset(scheduler_frame_layout, "s12");
		register_stack_offsets[RTOS_REG_CORTEX_M_D7] = lookup_field_offset(scheduler_frame_layout, "s14");
		register_stack_offsets[RTOS_REG_CORTEX_M_D8] = lookup_field_offset(scheduler_frame_layout, "s16");
		register_stack_offsets[RTOS_REG_CORTEX_M_D9] = lookup_field_offset(scheduler_frame_layout, "s18");
		register_stack_offsets[RTOS_REG_CORTEX_M_D10] = lookup_field_offset(scheduler_frame_layout, "s20");
		register_stack_offsets[RTOS_REG_CORTEX_M_D11] = lookup_field_offset(scheduler_frame_layout, "s22");
		register_stack_offsets[RTOS_REG_CORTEX_M_D12] = lookup_field_offset(scheduler_frame_layout, "s24");
		register_stack_offsets[RTOS_REG_CORTEX_M_D13] = lookup_field_offset(scheduler_frame_layout, "s26");
		register_stack_offsets[RTOS_REG_CORTEX_M_D14] = lookup_field_offset(scheduler_frame_layout, "s28");
		register_stack_offsets[RTOS_REG_CORTEX_M_D15] = lookup_field_offset(scheduler_frame_layout, "s30");
	}

	/* Along with the task layout */
	if (!task_layout) {
		gdb_api->log_output("RTOS-TOOLKIT: loading task layout from 0x%x\n", symbols[RTOS_SYMBOL_TASK_LAYOUT].address);
		task_layout = load_layout(symbols[RTOS_SYMBOL_TASK_LAYOUT].address);
		if (!task_layout) {
			gdb_api->error_output("failed to load task layout\n");
			return -ENOTSUP;
		}
	}

	/* Should we do an initial allocation for the tasks? */
	if (!tasks) {
		tasks_size = 8;
		struct rtostoolkit_task *new_tasks = gdb_api->alloc((tasks_size + 8)* sizeof(struct rtostoolkit_task));
		if (!new_tasks) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to allocate %u bytes for tasks\n", tasks_size * sizeof(struct rtostoolkit_task));
			num_tasks = 0;
			return -ENOMEM;
		}
		memcpy(new_tasks, tasks, tasks_size);
		gdb_api->free(tasks);
		tasks = new_tasks;
		tasks_size += 8;
	}

	/* Load the scheduler */
	status = gdb_api->read_uint32(symbols[0].address, &scheduler_ptr);
	if (status < 0) {
		gdb_api->error_output("RTOS-TOOLKIT: failed to read the scheduler pointer: %d\n", status);
		return status;
	}

	/* Check the marker  */
	if (scheduler_ptr) {

		/* Read the marker */
		uint32_t scheduler_marker;
		status = gdb_api->read_uint32(scheduler_ptr + lookup_field_offset(scheduler_layout, "marker"), &scheduler_marker);
		if (status < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the scheduler marker: %d\n", status);
			return status;
		}

		/* Ensure the scheduler pointer is null if not running or not valid, this handles the case where the first breakpoint is the Reset_Handler (i.e. before bss is zeroes */
		scheduler_ptr = scheduler_marker == RTOS_SCHEDULER_MARKER ? scheduler_ptr : 0;
	}

	/* If the scheduler is not running report one task with a default context */
	if (scheduler_ptr == 0) {
		tasks[0].task_id = 0;
		tasks[0].state = TASK_UNKNOWN;
		tasks[0].current_priority = 0;
		tasks[0].flags = 0;
		tasks[0].psp = 0;
		strcpy(tasks[0].name, "current");
		current_task = 0;
		num_tasks = 1;
		return 0;
	}

	/* Load the current task id */
	status = gdb_api->read_uint32(scheduler_ptr + lookup_field_offset(scheduler_layout, "current"), &current_task);
	if (status < 0) {
		gdb_api->error_output("RTOS-TOOLKIT: failed to read scheduler current task: %d\n", status);
		num_tasks = 0;
		return status;
	}

	/* If there is no current task then create a placeholder */
	if (!current_task) {
		tasks[0].task_id = 0;
		tasks[0].state = TASK_UNKNOWN;
		tasks[0].current_priority = 0;
		tasks[0].flags = 0;
		tasks[0].psp = 0;
		strcpy(tasks[0].name, "current");
		current_task = 0;
		num_tasks = 1;
	}

	/* Read the task list head ptr from the scheduler */
	uint32_t task_list_head_addr = scheduler_ptr + lookup_field_offset(scheduler_layout, "tasks.next");
	uint32_t task_addr;
	status = gdb_api->read_uint32(task_list_head_addr, &task_addr);
	if (status < 0) {
		gdb_api->error_output("RTOS-TOOLKIT: failed to read the scheduler task head addr 0x%x: %d\n", task_list_head_addr, status);
		return status;
	}

	/* Load all the tasks */
	while (task_addr != task_list_head_addr) {

		/* Adjust task addr to the start of the task structure */
		task_addr -= lookup_field_offset(task_layout, "scheduler_node.next");

		/* Read the task marker */
		uint32_t task_marker;
		status = gdb_api->read_uint32(task_addr + lookup_field_offset(task_layout, "marker"), &task_marker);
		if (status < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the task marker: %d\n", status);
			num_tasks = 0;
			return status;
		}

		/* Is the marker valid? */
		if (task_marker != RTOS_TASK_MARKER) {
			gdb_api->error_output("RTOS-TOOLKIT: confused, invalid task marker: 0x%x\n", task_marker);
			num_tasks = 0;
			return -EINVAL;
		}

		/* Do we need more memory for the tasks? */
		if (num_tasks >= tasks_size) {
			struct rtostoolkit_task *new_tasks = gdb_api->realloc(tasks, (tasks_size + 8) * sizeof(struct rtostoolkit_task));
			if (!new_tasks) {
				gdb_api->error_output("RTOS-TOOLKIT: failed to allocate %u bytes for tasks\n", tasks_size * sizeof(struct rtostoolkit_task));
				num_tasks = 0;
				return -ENOMEM;
			}
			tasks = new_tasks;
			tasks_size += 8;
		}

		/* Set the task id to the address */
		tasks[num_tasks].task_id = task_addr;

		/* Read the state */
		status = gdb_api->read_uint32(task_addr + lookup_field_offset(task_layout, "state"), (uint32_t *)&tasks[num_tasks].state);
		if (status < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the task state: %d\n", status);
			num_tasks = 0;
			return status;
		}

		/* Read the current priority */
		status = gdb_api->read_uint32(task_addr + lookup_field_offset(task_layout, "current_priority"), &tasks[num_tasks].current_priority);
		if (status < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the task current priority: %d\n", status);
			num_tasks = 0;
			return status;
		}

		/* Read the task name */
		status = gdb_api->read(task_addr + lookup_field_offset(task_layout, "name"), tasks[num_tasks].name, RTOS_TASK_NAME_LEN);
		if (status < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the task name: %d\n", status);
			num_tasks = 0;
			return status;
		}

		/* Read the task flags */
		status = gdb_api->read_uint32(task_addr + lookup_field_offset(task_layout, "flags"), &tasks[num_tasks].flags);
		if (status < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the task flags: %d\n", status);
			num_tasks = 0;
			return status;
		}

		/* If this is not the current task, read the task stack pointer */
		if (current_task != task_addr) {
			status = gdb_api->read_uint32(task_addr + lookup_field_offset(task_layout, "psp"), &tasks[num_tasks].psp);
			if (status < 0) {
				gdb_api->error_output("RTOS-TOOLKIT: failed to read the task flags: %d\n", status);
				num_tasks = 0;
				return -EIO;
			}
		} else
			tasks[num_tasks].psp = 0;

		/* Read the next task address */
		status = gdb_api->read_uint32(task_addr + lookup_field_offset(task_layout, "scheduler_node.next"), &task_addr);
		if (status < 0) {
			gdb_api->error_output("RTOS-TOOLKIT: failed to read the next task addr: %d\n", status);
			num_tasks = 0;
			return status;
		}

		/* Update the number of tasks */
		++num_tasks;
	}

	/* Sort the task list by task id */
	qsort(tasks, num_tasks, sizeof(struct rtostoolkit_task), compare_task_id);

	/* Ready to go */
	return 0;
}
