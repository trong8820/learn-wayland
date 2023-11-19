#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>

#include "xdg-shell.h"
#include "shm.h"

struct wl_data
{
	bool running;
	struct wl_shm        *wl_shm;
	struct wl_compositor *wl_compositor;
	struct xdg_wm_base   *xdg_wm_base;
	struct wl_buffer     *wl_buffer;
	struct wl_surface    *wl_surface;
};

static void noop()
{
}

static void wl_registry_global_handler(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
{
	struct wl_data *wl_data = data;
	printf("Got a registry event for %s id %d\n", interface, name);
	if      (strcmp(interface, wl_shm_interface.name) == 0)
	{
		wl_data->wl_shm = wl_registry_bind(wl_registry, name, &wl_shm_interface, version);
	}
	else if (strcmp(interface, wl_compositor_interface.name) == 0)
	{
		wl_data->wl_compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);
	}
	else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
	{
		wl_data->xdg_wm_base = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);
	}
}

static void wl_registry_global_remove_handler(void *data, struct wl_registry *wl_registry, uint32_t name)
{
	printf("Got a registry losing event for %d\n", name);
}

static const struct wl_registry_listener wl_registry_listener =
{
	.global = wl_registry_global_handler,
	.global_remove = wl_registry_global_remove_handler
};

static void wl_buffer_release_handler(void *data, struct wl_buffer *wl_buffer)
{
	wl_buffer_destroy(wl_buffer);
}

static const struct wl_buffer_listener wl_buffer_listener =
{
	.release = wl_buffer_release_handler
};

static void xdg_wm_base_ping_handler(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener =
{
	.ping = xdg_wm_base_ping_handler
};

static void xdg_surface_configure_handler(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	struct wl_data *wl_data = data;

	xdg_surface_ack_configure(xdg_surface, serial);

	wl_surface_attach(wl_data->wl_surface, wl_data->wl_buffer, 0, 0);
	wl_surface_commit(wl_data->wl_surface);
}

static const struct xdg_surface_listener xdg_surface_listener =
{
	.configure = xdg_surface_configure_handler
};

static void xdg_toplevel_configure_handler(void *data, struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height, struct wl_array *states)
{
}

static void xdg_toplevel_close_handler(void *data, struct xdg_toplevel *xdg_toplevel)
{
	struct wl_data *wl_data = data;
	wl_data->running = false;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener =
{
	.configure = xdg_toplevel_configure_handler,
	.close = xdg_toplevel_close_handler,
	.configure_bounds = noop,
	.wm_capabilities = noop
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

	struct wl_data wl_data = { 0 };
	wl_data.running = true;

	struct wl_registry *wl_registry = wl_display_get_registry(wl_display);
	wl_registry_add_listener(wl_registry, &wl_registry_listener, &wl_data);
	wl_display_roundtrip(wl_display);
	wl_registry_destroy(wl_registry);

	int32_t size = 640*480*4;
	int32_t fd = allocate_shm_file(size);
	uint32_t *buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	struct wl_shm_pool *wl_shm_pool = wl_shm_create_pool(wl_data.wl_shm, fd, size);
	wl_data.wl_buffer = wl_shm_pool_create_buffer(wl_shm_pool, 0, 640, 480, 640*4, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy(wl_shm_pool);
	close(fd);
	for (int y = 0; y < 480; ++y)
	{
		for (int x = 0; x < 640; ++x)
		{
			if ((x + y / 8 * 8) % 16 < 8)
			{
				buffer[y * 640 + x] = 0xFF666666;
			}
			else
			{
				buffer[y * 640 + x] = 0xFFEEEEEE;
			}
		}
	}
	munmap(buffer, size);
	// wl_buffer_add_listener(wl_data.wl_buffer, &wl_buffer_listener, NULL);

	xdg_wm_base_add_listener(wl_data.xdg_wm_base, &xdg_wm_base_listener, &wl_data);

	wl_data.wl_surface = wl_compositor_create_surface(wl_data.wl_compositor);
	struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(wl_data.xdg_wm_base, wl_data.wl_surface);

	xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, &wl_data);

	struct xdg_toplevel *xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
	xdg_toplevel_set_title(xdg_toplevel, "Learn Wayland");

	xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, &wl_data);

	wl_surface_commit(wl_data.wl_surface);

	while (wl_data.running == true && wl_display_dispatch(wl_display) != -1)
	{
	}

	xdg_toplevel_destroy(xdg_toplevel);
	xdg_surface_destroy(xdg_surface);
	wl_surface_destroy(wl_data.wl_surface);
	xdg_wm_base_destroy(wl_data.xdg_wm_base);
	wl_compositor_destroy(wl_data.wl_compositor);
	wl_shm_destroy(wl_data.wl_shm);
	wl_display_disconnect(wl_display);

	return EXIT_SUCCESS;
}
