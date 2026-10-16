#include <stdlib.h>
#include <string.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/server.h"
#include "log.h"
#include "stringop.h"
#include "util.h"

struct cmd_results *cmd_xwayland_scaling(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "xwayland_scaling", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

#if WLR_HAS_XWAYLAND
	if (strcmp(argv[0], "off") == 0) {
		config->xwayland_scaling = XWAYLAND_SCALING_OFF;
	} else if (strcmp(argv[0], "auto") == 0) {
		config->xwayland_scaling = XWAYLAND_SCALING_AUTO;
	} else {
		char *end;
		float factor = strtof(argv[0], &end);
		if (*end != '\0' || factor <= 0 || factor > 10) {
			return cmd_results_new(CMD_INVALID,
				"Expected 'xwayland_scaling off|auto|<factor>'");
		}
		config->xwayland_scaling = XWAYLAND_SCALING_FIXED;
		config->xwayland_scaling_factor = factor;
	}

	if (config->active && !config->validating) {
		xwayland_scale_update();
	}
#else
	sway_log(SWAY_INFO, "Ignoring `xwayland_scaling` command, "
		"sway hasn't been built with Xwayland support");
#endif

	return cmd_results_new(CMD_SUCCESS, NULL);
}

struct cmd_results *cmd_xwayland_scale_command(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "xwayland_scale_command", EXPECTED_AT_LEAST, 1))) {
		return error;
	}

	free(config->xwayland_scale_command);
	config->xwayland_scale_command = NULL;

	char *new_command = join_args(argv, argc);
	if (strcmp(new_command, "-") != 0) {
		config->xwayland_scale_command = new_command;
		sway_log(SWAY_DEBUG, "Using xwayland scale command: %s",
				config->xwayland_scale_command);
	} else {
		free(new_command);
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
