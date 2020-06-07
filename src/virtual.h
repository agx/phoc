#pragma once

#include <wayland-server-core.h>

void phoc_handle_virtual_keyboard(struct wl_listener *listener, void *data);
void phoc_handle_virtual_pointer(struct wl_listener *listener, void *data);
