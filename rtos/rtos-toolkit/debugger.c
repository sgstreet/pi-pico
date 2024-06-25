#include <rtos/rtos-toolkit/scheduler.h>

#define DEBUGGER_FIELD(type, member) { #member, offsetof(type, member), sizeof(((type *)0)->member) }

struct field_desc
{
	const char *name;
	uint32_t offset;
	uint32_t size;
};

const struct field_desc scheduler_frame_layout[] =
{
	DEBUGGER_FIELD(struct scheduler_frame, exec_return),
	DEBUGGER_FIELD(struct scheduler_frame, control),

	DEBUGGER_FIELD(struct scheduler_frame, r4),
	DEBUGGER_FIELD(struct scheduler_frame, r5),
	DEBUGGER_FIELD(struct scheduler_frame, r6),
	DEBUGGER_FIELD(struct scheduler_frame, r7),
	DEBUGGER_FIELD(struct scheduler_frame, r8),
	DEBUGGER_FIELD(struct scheduler_frame, r9),
	DEBUGGER_FIELD(struct scheduler_frame, r10),
	DEBUGGER_FIELD(struct scheduler_frame, r11),

	DEBUGGER_FIELD(struct scheduler_frame, r0),
	DEBUGGER_FIELD(struct scheduler_frame, r1),
	DEBUGGER_FIELD(struct scheduler_frame, r2),
	DEBUGGER_FIELD(struct scheduler_frame, r3),
	DEBUGGER_FIELD(struct scheduler_frame, r12),
	DEBUGGER_FIELD(struct scheduler_frame, lr),
	DEBUGGER_FIELD(struct scheduler_frame, pc),
	DEBUGGER_FIELD(struct scheduler_frame, psr),
	{ 0, 0, sizeof(struct scheduler_frame) },
};

static const struct field_desc task_layout[] =
{
	DEBUGGER_FIELD(struct task, psp),
	DEBUGGER_FIELD(struct task, state),
	DEBUGGER_FIELD(struct task, core),
	DEBUGGER_FIELD(struct task, current_priority),
//	DEBUGGER_FIELD(struct task, name),
	DEBUGGER_FIELD(struct task, scheduler_node.next),
	DEBUGGER_FIELD(struct task, marker),
	{ 0, 0, sizeof(struct task) },
};

static const struct field_desc scheduler_layout[] =
{
	DEBUGGER_FIELD(struct scheduler, tasks.next),
	DEBUGGER_FIELD(struct scheduler, marker),
	{ 0, 0, sizeof(struct scheduler) },
};

static const struct field_desc *layouts[] =
{
	task_layout,
	scheduler_layout,
	scheduler_frame_layout,
	0,
};

const void *_debugger_layouts = layouts;
