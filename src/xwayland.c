#define G_LOG_DOMAIN "phoc-xwayland"

#include "phoc-config.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/box.h>
#include <wlr/xwayland.h>
#include "cursor.h"
#include "server.h"
#include "seat.h"
#include "view.h"
#include "xwayland.h"
#include "xwayland-surface.h"

void handle_xwayland_surface(struct wl_listener *listener, void *data) {
	PhocDesktop *desktop =
		wl_container_of(listener, desktop, xwayland_surface);

	struct wlr_xwayland_surface *surface = data;
	g_debug ("new xwayland surface: title=%s, class=%s, instance=%s",
		surface->title, surface->class, surface->instance);
	wlr_xwayland_surface_ping(surface);

        /* Ref is dropped on surface destroy */
	phoc_xwayland_surface_new (surface);
}
