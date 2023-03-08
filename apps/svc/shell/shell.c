/*
 * shell.c
 *
 *  Created on: Feb 4, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include <compiler.h>
#include <container-of.h>

#include <rtos/rtos.h>
#include <sys/syslog.h>

#include <svc/shell.h>

#include "microrl.h"

extern void *__shell_cmd_start;
extern void *__shell_cmd_end;

struct shell
{
	osMutexId_t commands_lock;
	osThreadId_t shell_task;
	atomic_bool run;
	microrl_t microrl;
	char *completions[1];
	int num_commands;
	const struct shell_command *commands[];
};

struct shell *shell = 0;

static int shell_compare_command(const void *first, const void *second)
{
	struct shell_command *const *first_command = first;
	struct shell_command *const *second_command = second;

	return strcmp((*first_command)->name, (*second_command)->name);
}

static int shell_match_command(const void *first, const void *second)
{
	const char *name = first;
	struct shell_command *const *command = second;

	return strcmp(name, (*command)->name);
}

static struct shell_command *shell_find_command(struct shell *shell, const char *name)
{
	assert(shell != 0 && name != 0);

	struct shell_command **found = bsearch(name, shell->commands, shell->num_commands, sizeof(struct shell_cmd*), shell_match_command);
	if (!found)
		return 0;
	return *found;
}

static int shell_output(struct microrl* mrl, const char* str)
{
	return fputs(str, stdout);
}

static int shell_exec(struct microrl* mrl, int argc, const char* const *argv)
{
	assert(mrl != 0);

	struct shell *shell = container_of(mrl, struct shell, microrl);

	struct shell_command *cmd = shell_find_command(shell, argv[0]);
	if (!cmd) {
		printf("'%s' not found\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *cmd_line[argc];
	memcpy(cmd_line, argv, sizeof(char *) * argc);

	return cmd->func(argc, cmd_line);
}

static char **shell_get_completions(struct microrl* mrl, int argc, const char* const *argv)
{
	assert(mrl != 0);

	shell->completions[0] = 0;

	return shell->completions;
}

static __unused void shell_sigint(struct microrl* mrl)
{
	assert(mrl != 0);

	struct shell *shell = container_of(mrl, struct shell, microrl);

	shell->run = false;
}

static void shell_task(void *context)
{
	struct shell *shell = context;

	while (shell->run) {

		/* Wait for a character */
		int c = getchar();
		if (c != EOF)
			microrl_processing_input(&shell->microrl, (char *)&c, 1);
	}
}

static int help_main(int argc, char **argv)
{
	assert(shell != 0);

	for (size_t i = 0; i < shell->num_commands; ++i)
		printf("%s %s\n", shell->commands[i]->name, shell->commands[i]->usage);

	return 0;
}

static __shell_command const struct shell_command help_cmd =
{
	.name = "help",
	.usage = "",
	.func = help_main,
};

static __constructor_priority(SERVICES_PRIORITY) void shell_ini(void)
{
	/* Allocate the shell */
	shell = calloc(1, sizeof(struct shell) + sizeof(struct shell_command *) * (&__shell_cmd_end - &__shell_cmd_start));
	if (!shell)
		syslog_fatal("could not allocate shell\n");

	/* Load sort command pointer array */
	for (const struct shell_command *cmd = (const struct shell_command *)&__shell_cmd_start; cmd < (const struct shell_command *)&__shell_cmd_end; ++cmd)
		shell->commands[shell->num_commands++] = cmd;
	qsort(shell->commands, shell->num_commands, sizeof(struct shell_command *), shell_compare_command);

	/* Initialize the command line handler */
	microrlr_t microrl_status = microrl_init(&shell->microrl, shell_output, shell_exec);
	if (microrl_status != microrlOK)
		syslog_fatal("could not initialize microrl: %d\n", microrl_status);

	/* Bind completion handler */
	microrl_status = microrl_set_complete_callback(&shell->microrl, shell_get_completions);
	if (microrl_status != microrlOK)
		syslog_fatal("could not bind microrl completion handler: %d\n", microrl_status);

	/* Bind the control c handler
	microrl_status = microrl_set_sigint_callback(&shell->microrl, shell_sigint);
	if (microrl_status != microrlOK)
		syslog_fatal("could not bind microrl signal handler: %d\n", microrl_status);
	*/

	/* Initialize the the shell command lock */
	osMutexAttr_t mutex_attr = { .name = "commands-lock", .attr_bits = osMutexPrioInherit | osMutexRecursive };
	shell->commands_lock = osMutexNew(&mutex_attr);
	if (!shell->commands_lock)
		syslog_fatal("could not create shell commands lock\n");

	/* Launch the shell thread */
	shell->run = true;
	osThreadAttr_t thread_attr = { .name = "shell-task", .stack_size = SHELL_COMMAND_STACK_SIZE };
	shell->shell_task = osThreadNew(shell_task, shell, &thread_attr);
	if (!shell->shell_task)
		syslog_fatal("could not create shell task\n");
}

static __destructor_priority(SERVICES_PRIORITY) void shell_fini(void)
{
	assert(shell != 0);

	/* Stop the thread */
	shell->run = false;
	osStatus_t os_status = osThreadJoin(shell->shell_task);
	if (os_status != osOK) {
		syslog_error("failed to join shell task: %d\n", os_status);
		return;
	}

	/* Clear the shell */
	struct shell *old_shell = shell;
	shell = 0;

	/* Release the command lock */
	osMutexDelete(old_shell->commands_lock);

	/* Clean up the shell */
	free(old_shell);
}
