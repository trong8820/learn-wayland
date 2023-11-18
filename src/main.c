#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <wayland-client.h>

#include "xdg-shell.h"

struct client_state
{
	struct wl_surface *wl_surface;
	struct wl_compositor *wl_compositor;
	struct xdg_wm_base *xdg_wm_base;
};

static void xdg_wm_base_ping_handler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener =
{
	.ping = xdg_wm_base_ping_handler
};

static void wl_registry_global_handler(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
{
	struct client_state *state = data;
	printf("Got a registry event for %s id %d\n", interface, name);
	if      (strcmp(interface, wl_compositor_interface.name) == 0)
	{
		state->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
	{
		state->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
		xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
	}
}

static void wl_registry_global_remove_handler(void *data, struct wl_registry *wl_registry, uint32_t name)
{
	printf("Got a registry losing event for %d\n", name);
}

static void xdg_surface_configure_handler(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	struct client_state *state = data;

	xdg_surface_ack_configure(xdg_surface, serial);

	wl_surface_commit(state->wl_surface);
}

static const struct wl_registry_listener wl_registry_listener =
{
	.global = wl_registry_global_handler,
	.global_remove = wl_registry_global_remove_handler
};

static const struct xdg_surface_listener xdg_surface_listener =
{
	.configure = xdg_surface_configure_handler
};

int main()
{
	struct wl_display *wl_display = wl_display_connect(NULL);
	if (wl_display == NULL)
	{
		printf("Failed to connect to Wayland display.\n");
		return EXIT_FAILURE;
	}
	printf("Connection established!\n");

	struct client_state state = { 0 };

	struct wl_registry *wl_registry = wl_display_get_registry(wl_display);
	wl_registry_add_listener(wl_registry, &wl_registry_listener, &state);
	wl_display_roundtrip(wl_display);

	state.wl_surface = wl_compositor_create_surface(state.wl_compositor);
	struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.wl_surface);

	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, &state);

	struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
	xdg_toplevel_set_title(xdg_toplevel, "Learn Wayland");
	wl_surface_commit(state.wl_surface);

	//while (wl_display_dispatch(wl_display) != -1)
	{
	}

	wl_display_disconnect(wl_display);

	return EXIT_SUCCESS;
}
