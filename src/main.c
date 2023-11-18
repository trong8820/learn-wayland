#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wayland-client.h>

#include "xdg-shell.h"

int main()
{
	struct wl_display *display = wl_display_connect(NULL);
	if (display == NULL)
	{
		printf("Failed to connect to Wayland display.\n");
		return EXIT_FAILURE;
	}
	printf("Connection established!\n");

	wl_display_disconnect(display);

	return EXIT_SUCCESS;
}
