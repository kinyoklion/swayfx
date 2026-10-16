#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wlr/types/wlr_xdg_output_v1.h>
#include <wlr/util/server_scale.h>
#include <wlr/xwayland.h>
#include "log.h"
#include "sway/config.h"
#include "sway/output.h"
#include "sway/server.h"
#include "sway/tree/container.h"
#include "sway/tree/root.h"
#include "sway/tree/view.h"
#include "sway/xwayland.h"

double xwayland_scale(void) {
	return server.xwayland_scale > 0 ? server.xwayland_scale : 1.0;
}

double xwayland_scale_to_x(double logical) {
	return round(logical * xwayland_scale());
}

double xwayland_scale_from_x(double x_native) {
	return x_native / xwayland_scale();
}

static double compute_scale(void) {
	switch (config->xwayland_scaling) {
	case XWAYLAND_SCALING_OFF:
		return 1.0;
	case XWAYLAND_SCALING_FIXED:
		return config->xwayland_scaling_factor;
	case XWAYLAND_SCALING_AUTO:;
		double max = 1.0;
		for (int i = 0; i < root->outputs->length; ++i) {
			struct sway_output *output = root->outputs->items[i];
			if (output->wlr_output->scale > max) {
				max = output->wlr_output->scale;
			}
		}
		return max;
	}
	return 1.0;
}

static void spawn_scale_command(double scale) {
	if (!config->xwayland_scale_command) {
		return;
	}
	pid_t child = fork();
	if (child == 0) {
		setsid();
		char scale_str[32];
		snprintf(scale_str, sizeof(scale_str), "%g", scale);
		setenv("SWAY_XWAYLAND_SCALE", scale_str, 1);
		execlp("sh", "sh", "-c", config->xwayland_scale_command, (void *)NULL);
		sway_log_errno(SWAY_ERROR, "execlp failed");
		_exit(1);
	} else if (child < 0) {
		sway_log_errno(SWAY_ERROR, "fork failed");
	}
}

static void rescale_view_iter(struct sway_container *con, void *data) {
	struct sway_view *view = con->view;
	if (view == NULL || view->type != SWAY_VIEW_XWAYLAND ||
			view->surface == NULL) {
		return;
	}
	// Logical geometry is unchanged, but its X11-native (pixel) equivalent is
	// not: push a configure through so Xwayland resizes the real windows.
	view->impl->configure(view, con->current.content_x, con->current.content_y,
		con->current.content_width, con->current.content_height);
}

void xwayland_scale_handle_ready(void) {
	// Force the full apply path (xdg-output resend, scale command) even if
	// the scale value is unchanged: the X server and wl_client are new.
	server.xwayland_scale = 0;
	xwayland_scale_update();
}

void xwayland_scale_update(void) {
	struct wlr_xwayland *xwayland = server.xwayland.wlr_xwayland;
	if (xwayland == NULL) {
		return;
	}

	double scale = compute_scale();

	// Always (re-)register the client: it may be new after an Xwayland
	// restart even when the scale value itself is unchanged.
	if (xwayland->server != NULL && xwayland->server->client != NULL) {
		wlr_server_scale_set(xwayland->server->client, scale);
	}

	if (scale == server.xwayland_scale) {
		return;
	}
	sway_log(SWAY_INFO, "Xwayland scale changed %f -> %f",
		server.xwayland_scale, scale);
	server.xwayland_scale = scale;

	wlr_xdg_output_manager_v1_resend(server.xdg_output_manager_v1);
	root_for_each_container(rescale_view_iter, NULL);
	spawn_scale_command(scale);
}
