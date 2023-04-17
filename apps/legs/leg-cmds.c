/*
 * leg-cmds.c
 *
 *  Created on: Apr 7, 2023
 *      Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <svc/shell.h>

#include <legs/leg-manager.h>

static int leg_validate_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	/* Get the leg manager */
	struct leg_manager *manager = leg_manager_get_instance();
	if (!manager) {
		fprintf(stderr, "failed to ge the leg manager instance\n");
		return EXIT_FAILURE;
	}

	/* Do we have at leg id arg? */
	if (argc < 2) {
		fprintf(stderr, "missing parameters\n");
		return EXIT_FAILURE;
	}

	/* Convert the led id */
	errno = 0;
	char *end;
	unsigned int leg_id = strtoul(argv[1], &end, 0);
	if (errno != 0 || end == argv[1] || leg_id >= leg_manager_get_instance()->num_legs) {
		printf("bad leg id: '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* The leg to validate */
	struct leg *leg = leg_manager_get_leg(manager, leg_id);
	if (!leg) {
		fprintf(stderr, "failed to get leg %u\n", leg_id);
		return EXIT_FAILURE;
	}

	/* Validate the leg */
	int status = leg_validate(leg);
	printf("leg %u is%s valid\n", leg_id, (status < 0 ? " not" : ""));

	return status < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

static __shell_command const struct shell_command _leg_validate_cmd =
{
	.name = "leg-validate",
	.usage = "<leg id>",
	.func = leg_validate_cmd,
};

static int leg_configure_cmd(int argc, char **argv)
{
	assert(argc >= 1 && argv != 0);

	/* Get the leg manager */
	struct leg_manager *manager = leg_manager_get_instance();
	if (!manager) {
		fprintf(stderr, "failed to ge the leg manager instance\n");
		return EXIT_FAILURE;
	}

	/* Do we have at leg id arg? */
	if (argc < 2) {
		fprintf(stderr, "missing parameters\n");
		return EXIT_FAILURE;
	}

	/* Convert the led id */
	errno = 0;
	char *end;
	unsigned int leg_id = strtoul(argv[1], &end, 0);
	if (errno != 0 || end == argv[1] || leg_id >= leg_manager_get_instance()->num_legs) {
		printf("bad leg id: '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* The leg to validate */
	struct leg *leg = leg_manager_get_leg(manager, leg_id);
	if (!leg) {
		fprintf(stderr, "failed to get leg %u\n", leg_id);
		return EXIT_FAILURE;
	}

	/* Validate the leg */
	int status = leg_configure(leg, 0);
	printf("leg %u is%s configured\n", leg_id, (status < 0 ? " not" : ""));

	return status < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

static __shell_command const struct shell_command _leg_configure_cmd =
{
	.name = "leg-configure",
	.usage = "<leg id>",
	.func = leg_configure_cmd,
};
