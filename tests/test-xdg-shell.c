/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

typedef struct _PhocTestXdgToplevelSurface
{
  struct wl_surface *wl_surface;
  struct xdg_surface *xdg_surface;
  struct xdg_toplevel *xdg_toplevel;
  PhocTestBuffer buffer;
  guint32 width, height;
  gboolean configured;
  gboolean toplevel_configured;
} PhocTestXdgToplevelSurface;

static void
xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface,
			     uint32_t serial)
{
  PhocTestXdgToplevelSurface *xs = data;

  g_debug ("Configured %p serial %d", xdg_surface, serial);
  xdg_surface_ack_configure(xs->xdg_surface, serial);
  xs->configured = TRUE;
}

static const struct xdg_surface_listener xdg_surface_listener = {
  xdg_surface_handle_configure,
};

#define WIDTH 100
#define HEIGHT 200

static void
xdg_toplevel_handle_configure(void *data, struct xdg_toplevel *xdg_toplevel,
			      int32_t width, int32_t height,
			      struct wl_array *states)
{
  PhocTestXdgToplevelSurface *xs = data;

  g_debug ("Configured %p, size: %dx%d", xdg_toplevel, width, height);
  g_assert_nonnull (xdg_toplevel);

  xs->width = width ?: WIDTH;
  xs->height = height ?: HEIGHT;
  xs->toplevel_configured = TRUE;
}

static void
xdg_toplevel_handle_close(void *data, struct xdg_toplevel *xdg_surface)
{
  /* TBD */
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	xdg_toplevel_handle_configure,
	xdg_toplevel_handle_close,
};

static void
phoc_test_xdg_surface_free (PhocTestXdgToplevelSurface *xs)
{
  xdg_toplevel_destroy (xs->xdg_toplevel);
  xdg_surface_destroy (xs->xdg_surface);
  wl_surface_destroy (xs->wl_surface);
  phoc_test_buffer_free (&xs->buffer);
  g_free (xs);
}

static PhocTestXdgToplevelSurface *
phoc_test_xdg_surface_new (PhocTestClientGlobals *globals,
			   guint32 width, guint32 height, guint32 color)
{
  PhocTestXdgToplevelSurface *xs = g_malloc0 (sizeof(PhocTestXdgToplevelSurface));

  xs->wl_surface = wl_compositor_create_surface (globals->compositor);
  g_assert_nonnull (xs->wl_surface);

    xs->xdg_surface = xdg_wm_base_get_xdg_surface (globals->xdg_shell,
						   xs->wl_surface);
  g_assert_nonnull (xs->wl_surface);
  xdg_surface_add_listener (xs->xdg_surface,
			    &xdg_surface_listener, xs);
    xs->xdg_toplevel = xdg_surface_get_toplevel (xs->xdg_surface);
  g_assert_nonnull (xs->xdg_toplevel);
  xdg_toplevel_add_listener (xs->xdg_toplevel,
			     &xdg_toplevel_listener, xs);
  xdg_toplevel_set_min_size (xs->xdg_toplevel, WIDTH, HEIGHT);

  wl_surface_commit (xs->wl_surface);
  wl_display_dispatch (globals->display);
  wl_display_roundtrip (globals->display);

  g_assert_true (xs->configured);
  g_assert_true (xs->toplevel_configured);

  phoc_test_client_create_shm_buffer (globals, &xs->buffer, xs->width, xs->height,
				      WL_SHM_FORMAT_XRGB8888);

  for (int i = 0; i < xs->width * xs->height * 4; i+=4)
    *(guint32*)(xs->buffer.shm_data + i) = color;

  wl_surface_attach (xs->wl_surface, xs->buffer.wl_buffer, 0, 0);
  wl_surface_damage (xs->wl_surface, 0, 0, xs->width, xs->height);
  wl_surface_commit (xs->wl_surface);
  wl_display_dispatch (globals->display);
  wl_display_roundtrip (globals->display);

  return xs;
}

static gboolean
test_client_xdg_shell_normal (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestXdgToplevelSurface *ls_green;

  ls_green = phoc_test_xdg_surface_new (globals, WIDTH, HEIGHT, 0xFF00FF00);
  g_assert_nonnull (ls_green);
  phoc_assert_screenshot (globals, "test-xdg-shell-normal-1.png");

  phoc_test_xdg_surface_free (ls_green);
  phoc_assert_screenshot (globals, "empty.png");

  return TRUE;
}

static gboolean
test_client_xdg_shell_maximized (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestXdgToplevelSurface *ls_green;

  ls_green = phoc_test_xdg_surface_new (globals, WIDTH, HEIGHT, 0xFF00FF00);
  g_assert_nonnull (ls_green);
  phoc_assert_screenshot (globals, "test-xdg-shell-maximized-1.png");

  phoc_test_xdg_surface_free (ls_green);
  phoc_assert_screenshot (globals, "empty.png");

  return TRUE;
}

static gboolean
test_client_xdg_shell_server_prepare (PhocServer *server, gpointer data)
{
  PhocDesktop *desktop = server->desktop;
  gboolean maximize = GPOINTER_TO_INT (data);

  g_assert_nonnull (desktop);
  phoc_desktop_set_auto_maximize (desktop, maximize);
  return TRUE;
}

static void
test_xdg_shell_normal (void)
{
  PhocTestClientIface iface = {
   .server_prepare = test_client_xdg_shell_server_prepare,
   .client_run     = test_client_xdg_shell_normal,
  };

  phoc_test_client_run (3, &iface, GINT_TO_POINTER (FALSE));
}

static void
test_xdg_shell_maximized (void)
{
  PhocTestClientIface iface = {
   .server_prepare = test_client_xdg_shell_server_prepare,
   .client_run     = test_client_xdg_shell_maximized,
  };

  phoc_test_client_run (3, &iface, GINT_TO_POINTER (TRUE));
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phoc/xdg-shell/simple", test_xdg_shell_normal);
  g_test_add_func("/phoc/xdg-shell/maximize", test_xdg_shell_maximized);
  return g_test_run();
}
