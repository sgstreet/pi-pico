#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <alloca.h>

#include <init/init-sections.h>
#include <rtos/rtos.h>

struct arguments
{
	int argc;
	char **argv;
	int ret;
};

int _main(int argc, char **argv);
int main(int argc, char **argv);

static void main_task(void *context)
{
	assert(context != 0);

	struct arguments *args = context;

	/* Initialize the iso thread, this will wrap the main task */
	args->ret = main(args->argc, args->argv);

	/* Must terminate in order to not fall off the edge of the world */
	scheduler_terminate(0);
}

static void task_exit_handler(struct task *task)
{
	assert(task != 0);
	printf("%s exited\n", task->name);
}

int _main(int argc, char **argv)
{
	struct scheduler scheduler;

	/* Initialize the scheduler */
	int status = scheduler_init(&scheduler, (size_t)&__tls_size);
	if (status < 0)
		return status;

	/* Execute the ini array */
	run_init_array(__init_array_start, __init_array_end);

	/* Setup the main task */
	struct arguments args = { .argc = argc, .argv = argv, .ret = 0 };
	struct task_descriptor main_task_descriptor;
	strcpy(main_task_descriptor.name, "main-task");
	main_task_descriptor.entry_point = main_task;
	main_task_descriptor.exit_handler = task_exit_handler;
	main_task_descriptor.context = &args;
	main_task_descriptor.flags = 0;
	main_task_descriptor.priority = SCHEDULER_MAX_TASK_PRIORITY;
	struct task *main_task = scheduler_create(alloca(SCHEDULER_MAIN_STACK_SIZE), SCHEDULER_MAIN_STACK_SIZE, &main_task_descriptor);
	if (!main_task) {
		errno = EINVAL;
		return -EINVAL;
	}

	/* Run the scheduler */
	status = scheduler_run();
	if (status == 0)
		status = args.ret;

	/* Execute the fini array */
	run_init_array(__fini_array_start, __fini_array_end);

	return status;
}

int main(int argc, char **argv)
{
	int i = 0;
	printf("hello world! %d\n", i);
	return 0xdeadbeef;
}
