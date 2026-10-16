#ifndef SWAY_XWAYLAND_H
#define SWAY_XWAYLAND_H

#include <wlr/xwayland.h>
#include <xcb/xproto.h>

enum atom_name {
	NET_WM_WINDOW_TYPE_NORMAL,
	NET_WM_WINDOW_TYPE_DIALOG,
	NET_WM_WINDOW_TYPE_UTILITY,
	NET_WM_WINDOW_TYPE_TOOLBAR,
	NET_WM_WINDOW_TYPE_SPLASH,
	NET_WM_WINDOW_TYPE_MENU,
	NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
	NET_WM_WINDOW_TYPE_POPUP_MENU,
	NET_WM_WINDOW_TYPE_TOOLTIP,
	NET_WM_WINDOW_TYPE_NOTIFICATION,
	NET_WM_STATE_MODAL,
	ATOM_LAST,
};

struct sway_xwayland {
	struct wlr_xwayland *wlr_xwayland;
	struct wlr_xcursor_manager *xcursor_manager;

	xcb_atom_t atoms[ATOM_LAST];
};

void handle_xwayland_ready(struct wl_listener *listener, void *data);

// Current global Xwayland scale factor S; X11-native units = logical * S.
// 1.0 unless xwayland_scaling is enabled.
double xwayland_scale(void);

// Convert between logical and X11-native coordinates.
double xwayland_scale_to_x(double logical);
double xwayland_scale_from_x(double x_native);

// Recompute S from config + outputs and apply it (server scale override,
// xdg-output resend, resize sweep, scale command). Safe to call any time.
void xwayland_scale_update(void);

// Like xwayland_scale_update(), but forces the full apply path; called from
// the Xwayland ready handler (new X server / wl_client).
void xwayland_scale_handle_ready(void);

#endif
