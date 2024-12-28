/*
 * Copyright (C) 2024 The Phosh Developres
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "cursor.h"
#include "desktop.h"
#include "desktop-xwayland.h"
#include "server.h"
#include "xwayland-surface.h"


#ifdef PHOC_XWAYLAND
static const char *atom_map[XWAYLAND_ATOM_LAST] = {
        "_NET_WM_WINDOW_TYPE_NORMAL",
        "_NET_WM_WINDOW_TYPE_DIALOG"
};

static void
handle_xwayland_ready (struct wl_listener *listener,
                       void               *data)
{
  PhocInput *input = phoc_server_get_input (phoc_server_get_default ());
  PhocDesktop *desktop = wl_container_of (listener, desktop, xwayland_ready);
  xcb_connection_t *xcb_conn = xcb_connect (NULL, NULL);

  int err = xcb_connection_has_error (xcb_conn);
  if (err) {
    g_warning ("XCB connect failed: %d", err);
    return;
  }

  xcb_intern_atom_cookie_t cookies[XWAYLAND_ATOM_LAST];

  for (size_t i = 0; i < XWAYLAND_ATOM_LAST; i++)
    cookies[i] = xcb_intern_atom (xcb_conn, 0, strlen (atom_map[i]), atom_map[i]);

  for (size_t i = 0; i < XWAYLAND_ATOM_LAST; i++) {
    xcb_generic_error_t *error = NULL;
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply (xcb_conn, cookies[i], &error);

    if (error) {
      g_warning ("could not resolve atom %s, X11 error code %d",
                 atom_map[i], error->error_code);
      free (error);
    }

    if (reply)
      desktop->xwayland_atoms[i] = reply->atom;

    free (reply);
  }

  xcb_disconnect (xcb_conn);

  if (desktop->xwayland != NULL) {
    PhocSeat *xwayland_seat = phoc_input_get_seat (input, PHOC_CONFIG_DEFAULT_SEAT_NAME);
    wlr_xwayland_set_seat (desktop->xwayland, xwayland_seat->seat);
  }
}


static void
handle_xwayland_remove_startup_id (struct wl_listener *listener, void *data)
{
  PhocDesktop *desktop = wl_container_of (listener, desktop, xwayland_remove_startup_id);
  struct wlr_xwayland_remove_startup_info_event *ev = data;

  g_assert (PHOC_IS_DESKTOP (desktop));
  g_assert (ev->id);

  phoc_phosh_private_notify_startup_id (phoc_desktop_get_phosh_private (desktop),
                                        ev->id,
                                        PHOSH_PRIVATE_STARTUP_TRACKER_PROTOCOL_X11);
}


static void
handle_xwayland_surface (struct wl_listener *listener, void *data)
{
  struct wlr_xwayland_surface *surface = data;
  g_debug ("new xwayland surface: title=%s, class=%s, instance=%s",
           surface->title, surface->class, surface->instance);
  wlr_xwayland_surface_ping(surface);

  /* Ref is dropped on surface destroy */
  phoc_xwayland_surface_new (surface);
}

#endif /* PHOC_XWAYLAND */

void
phoc_desktop_setup_xwayland (PhocDesktop *self)
{
#ifdef PHOC_XWAYLAND
  const char *cursor_default = PHOC_XCURSOR_DEFAULT;
  PhocServer *server = phoc_server_get_default ();
  PhocConfig *config = phoc_server_get_config (server);

  self->xcursor_manager = wlr_xcursor_manager_create (NULL, PHOC_XCURSOR_SIZE);
  g_return_if_fail (self->xcursor_manager);

  if (config->xwayland) {
    struct wl_display *wl_display = phoc_server_get_wl_display (server);
    struct wlr_compositor *wlr_compositor = phoc_server_get_compositor (server);

    self->xwayland = wlr_xwayland_create (wl_display, wlr_compositor, config->xwayland_lazy);
    if (!self->xwayland) {
      g_critical ("Failed to initialize Xwayland");
      g_unsetenv ("DISPLAY");
      return;
    }

    wl_signal_add (&self->xwayland->events.new_surface, &self->xwayland_surface);
    self->xwayland_surface.notify = handle_xwayland_surface;

    wl_signal_add (&self->xwayland->events.ready, &self->xwayland_ready);
    self->xwayland_ready.notify = handle_xwayland_ready;

    wl_signal_add (&self->xwayland->events.remove_startup_info, &self->xwayland_remove_startup_id);
    self->xwayland_remove_startup_id.notify = handle_xwayland_remove_startup_id;

    g_setenv ("DISPLAY", self->xwayland->display_name, true);

    if (!wlr_xcursor_manager_load (self->xcursor_manager, 1))
      g_critical ("Cannot load XWayland XCursor theme");

    struct wlr_xcursor *xcursor = wlr_xcursor_manager_get_xcursor (self->xcursor_manager,
                                                                   cursor_default,
                                                                   1);
    if (xcursor != NULL) {
      struct wlr_xcursor_image *image = xcursor->images[0];
      wlr_xwayland_set_cursor (self->xwayland, image->buffer,
                               image->width * 4, image->width, image->height, image->hotspot_x,
                               image->hotspot_y);
    }
  }
#endif
}
