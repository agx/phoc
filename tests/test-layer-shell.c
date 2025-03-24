/*
 * Copyright (C) 2020 Purism SPC
 *               2025 The Phosh Devlopers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "testlib.h"

#include <wayland-client-protocol.h>

typedef struct _PhocTestLayerSurface {
  struct wl_surface            *wl_surface;
  struct zwlr_layer_surface_v1 *layer_surface;
  PhocTestBuffer                buffer;
  guint32                       width, height;
  gboolean                      configured;
} PhocTestLayerSurface;


typedef struct _PhocTestPopup {
  struct wl_surface            *wl_surface;
  struct xdg_surface           *xdg_surface;
  struct xdg_popup             *xdg_popup;
  PhocTestBuffer                buffer;
  guint32                       width, height;
  gboolean                      configured;
  guint32                       repositioned;
} PhocTestPopup;


static void
handle_xdg_popup_configure (void             *data,
                            struct xdg_popup *xdg_popup,
                            int32_t           x,
                            int32_t           y,
                            int32_t           width,
                            int32_t           height)
{
  PhocTestPopup *popup = data;

  popup->width = width;
  popup->height = height;
  popup->configured = TRUE;
}


static void
phoc_test_popup_destroy (PhocTestPopup *popup)
{
  xdg_popup_destroy (popup->xdg_popup);
  popup->xdg_popup = NULL;
  xdg_surface_destroy (popup->xdg_surface);
  popup->xdg_surface = NULL;
  wl_surface_destroy (popup->wl_surface);
  popup->wl_surface = NULL;
  phoc_test_buffer_free (&popup->buffer);

  g_free (popup);
}


static void
handle_xdg_popup_done (void *data, struct xdg_popup *xdg_popup)
{
  PhocTestPopup *popup = data;

  phoc_test_popup_destroy (popup);
}


static void
handle_xdg_popup_repositioned (void             *data,
                               struct xdg_popup *xdg_popup,
                               uint32_t          token)
{
  PhocTestPopup *popup = data;

  popup->repositioned = token;
}


static const struct xdg_popup_listener xdg_popup_listener = {
  .configure = handle_xdg_popup_configure,
  .popup_done = handle_xdg_popup_done,
  .repositioned = handle_xdg_popup_repositioned,
};


static void
handle_xdg_surface_handle_configure (void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
  xdg_surface_ack_configure (xdg_surface, serial);
}


static const struct xdg_surface_listener xdg_surface_listener = {
  .configure = handle_xdg_surface_handle_configure,
};


static PhocTestPopup *
phoc_test_popup_new (PhocTestClientGlobals *globals,
                     guint32                x,
                     guint32                y,
                     guint32                width,
                     guint32                height,
                     guint32                color,
                     PhocTestLayerSurface  *parent)

{
  struct xdg_positioner *xdg_positioner;
  PhocTestPopup *popup = g_new0 (PhocTestPopup, 1);

  popup->wl_surface = wl_compositor_create_surface (globals->compositor);
  g_assert_nonnull (popup->wl_surface);
  popup->xdg_surface =  xdg_wm_base_get_xdg_surface (globals->xdg_shell, popup->wl_surface);
  g_assert_nonnull (popup->xdg_surface);
  xdg_positioner = xdg_wm_base_create_positioner (globals->xdg_shell);
  g_assert_nonnull (xdg_positioner);

  xdg_positioner_set_size (xdg_positioner, width, height);
  xdg_positioner_set_offset (xdg_positioner, 0, 0);
  xdg_positioner_set_anchor_rect (xdg_positioner, x, y, 1, 1);
  xdg_positioner_set_anchor (xdg_positioner, XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT);

  popup->xdg_popup = xdg_surface_get_popup (popup->xdg_surface, NULL, xdg_positioner);
  g_assert_nonnull (popup->xdg_popup);

  zwlr_layer_surface_v1_get_popup (parent->layer_surface, popup->xdg_popup);

  xdg_surface_add_listener (popup->xdg_surface, &xdg_surface_listener, popup);
  xdg_popup_add_listener (popup->xdg_popup, &xdg_popup_listener, popup);

  wl_surface_commit (popup->wl_surface);
  wl_display_roundtrip (globals->display);

  xdg_positioner_destroy (xdg_positioner);

  g_assert_true (popup->configured);
  phoc_test_client_create_shm_buffer (globals,
                                      &popup->buffer,
                                      popup->width,
                                      popup->height,
                                      WL_SHM_FORMAT_XRGB8888);

  for (int i = 0; i < popup->width * popup->height * 4; i += 4)
    *(guint32*)(popup->buffer.shm_data + i) = color;

  wl_surface_attach (popup->wl_surface, popup->buffer.wl_buffer, 0, 0);
  wl_surface_damage (popup->wl_surface, 0, 0, popup->width, popup->height);
  wl_surface_commit (popup->wl_surface);
  wl_display_dispatch (globals->display);
  wl_display_roundtrip (globals->display);

  return popup;
}


static void
phoc_test_popup_reposition (PhocTestClientGlobals *globals, PhocTestPopup *popup)
{
  struct xdg_positioner *xdg_positioner;
  guint token = 0x1234;

  xdg_positioner = xdg_wm_base_create_positioner (globals->xdg_shell);
  g_assert_nonnull (xdg_positioner);
  /* Reposition a bit */
  xdg_positioner_set_size (xdg_positioner, popup->width, popup->height);
  xdg_positioner_set_offset (xdg_positioner, 10, 10);
  xdg_positioner_set_anchor_rect (xdg_positioner, 1, 1, 1, 1);
  xdg_positioner_set_anchor (xdg_positioner, XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT);

  xdg_popup_reposition (popup->xdg_popup, xdg_positioner, token);
  wl_display_dispatch (globals->display);
  wl_display_roundtrip (globals->display);
  g_assert_cmpint (popup->repositioned, ==, token);

  xdg_positioner_destroy (xdg_positioner);
}


static void
layer_surface_configure (void                         *data,
                         struct zwlr_layer_surface_v1 *surface,
                         uint32_t                      serial,
                         uint32_t                      width,
                         uint32_t                      height)
{
  PhocTestLayerSurface *ls = data;

  g_debug ("Configured %p serial %d", surface, serial);
  g_assert_cmpint (serial, >, 0);
  g_assert_nonnull (surface);
  g_assert_cmpint (width, >, 0);
  g_assert_cmpint (height, >, 0);
  zwlr_layer_surface_v1_ack_configure (surface, serial);

  ls->width = width;
  ls->height = height;

  ls->configured = TRUE;
}


static void
layer_surface_closed (void                         *data,
                      struct zwlr_layer_surface_v1 *surface)
{
  g_debug ("Destroyed %p", surface);
  zwlr_layer_surface_v1_destroy (surface);
}


static struct zwlr_layer_surface_v1_listener layer_surface_listener = {
  .configure = layer_surface_configure,
  .closed = layer_surface_closed,
};

static PhocTestLayerSurface *
phoc_test_layer_surface_new (PhocTestClientGlobals *globals,
                             guint32 width, guint32 height, guint32 color,
                             guint32 anchor, guint32 exclusive_zone)
{
  PhocTestLayerSurface *ls = g_malloc0 (sizeof(PhocTestLayerSurface));

  ls->wl_surface = wl_compositor_create_surface (globals->compositor);
  g_assert_nonnull (ls->wl_surface);

  ls->layer_surface = zwlr_layer_shell_v1_get_layer_surface (globals->layer_shell,
                                                             ls->wl_surface,
                                                             NULL,
                                                             ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                                                             "phoc-test");
  g_assert_nonnull (ls->wl_surface);
  zwlr_layer_surface_v1_set_size (ls->layer_surface, width, height);
  zwlr_layer_surface_v1_set_exclusive_zone (ls->layer_surface, exclusive_zone);
  zwlr_layer_surface_v1_add_listener (ls->layer_surface,
                                      &layer_surface_listener,
                                      ls);
  zwlr_layer_surface_v1_set_anchor (ls->layer_surface, anchor);
  wl_surface_commit (ls->wl_surface);
  wl_display_dispatch (globals->display);
  wl_display_roundtrip (globals->display);
  g_assert_true (ls->configured);

  phoc_test_client_create_shm_buffer (globals, &ls->buffer, ls->width, ls->height,
                                      WL_SHM_FORMAT_XRGB8888);

  for (int i = 0; i < ls->width * ls->height * 4; i += 4)
    *(guint32*)(ls->buffer.shm_data + i) = color;

  wl_surface_attach (ls->wl_surface, ls->buffer.wl_buffer, 0, 0);
  wl_surface_damage (ls->wl_surface, 0, 0, ls->width, ls->height);
  wl_surface_commit (ls->wl_surface);
  wl_display_dispatch (globals->display);
  wl_display_roundtrip (globals->display);

  return ls;
}

static void
phoc_test_layer_surface_free (PhocTestLayerSurface *ls)
{
  zwlr_layer_surface_v1_destroy (ls->layer_surface);
  wl_surface_destroy (ls->wl_surface);
  phoc_test_buffer_free (&ls->buffer);
  g_free (ls);
}

#define WIDTH 100
#define HEIGHT 200

static gboolean
test_client_layer_shell_anchor (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestLayerSurface *ls_green, *ls_red;

  ls_green = phoc_test_layer_surface_new (globals, WIDTH, HEIGHT, 0xFF00FF00,
                                          ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP, 0);
  g_assert_nonnull (ls_green);
  phoc_assert_screenshot (globals, "test-layer-shell-anchor-1.png");

  ls_red = phoc_test_layer_surface_new (globals, WIDTH * 2, HEIGHT * 2, 0xFFFF0000,
                                        ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM, 0);
  g_assert_nonnull (ls_red);
  phoc_assert_screenshot (globals, "test-layer-shell-anchor-2.png");

  phoc_test_layer_surface_free (ls_green);
  phoc_test_layer_surface_free (ls_red);

  phoc_assert_screenshot (globals, "empty.png");
  return TRUE;
}


static void
test_layer_shell_anchor (void)
{
  PhocTestClientIface iface = { .client_run =  test_client_layer_shell_anchor };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}

static gboolean
test_client_layer_shell_exclusive_zone (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestLayerSurface *ls_green, *ls_red;

  ls_green = phoc_test_layer_surface_new (globals, 0, HEIGHT, 0xFF00FF00,
                                          ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                                          | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
                                          | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                                          HEIGHT);
  g_assert_nonnull (ls_green);
  phoc_assert_screenshot (globals, "test-layer-shell-exclusive-zone-1.png");

  ls_red = phoc_test_layer_surface_new (globals, WIDTH * 2, HEIGHT * 2, 0xFFFF0000,
                                        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP, 0);
  g_assert_nonnull (ls_red);
  phoc_assert_screenshot (globals, "test-layer-shell-exclusive-zone-2.png");

  phoc_test_layer_surface_free (ls_red);
  phoc_test_layer_surface_free (ls_green);

  phoc_assert_screenshot (globals, "empty.png");
  return TRUE;
}

static void
test_layer_shell_exclusive_zone (void)
{
  PhocTestClientIface iface = { .client_run = test_client_layer_shell_exclusive_zone };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


static gboolean
test_client_layer_shell_set_layer (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestLayerSurface *ls_green, *ls_red;

  ls_green = phoc_test_layer_surface_new (globals, 0, HEIGHT, 0xFF00FF00,
                                          ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                                          | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
                                          | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                                          0);
  g_assert_nonnull (ls_green);

  ls_red = phoc_test_layer_surface_new (globals, WIDTH * 2, HEIGHT * 2, 0xFFFF0000,
                                        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP, 0);
  g_assert_nonnull (ls_red);

  /* Red layer is above green one as this is rendered last */
  phoc_assert_screenshot (globals, "test-layer-shell-set-layer-1.png");

  zwlr_layer_surface_v1_set_layer (ls_red->layer_surface, ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM);
  wl_surface_commit (ls_red->wl_surface);

  /* Green layer is above red one as red one moved to bottom */
  phoc_assert_screenshot (globals, "test-layer-shell-set-layer-2.png");

  phoc_test_layer_surface_free (ls_red);
  phoc_test_layer_surface_free (ls_green);

  phoc_assert_screenshot (globals, "empty.png");
  return TRUE;
}


static void
test_layer_shell_set_layer (void)
{
  PhocTestClientIface iface = { .client_run = test_client_layer_shell_set_layer };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


static gboolean
test_client_layer_shell_popup (PhocTestClientGlobals *globals, gpointer data)
{
  PhocTestLayerSurface *ls_green;
  PhocTestPopup *popup;

  ls_green = phoc_test_layer_surface_new (globals, 0, HEIGHT, 0xFF00FF00,
                                          ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
                                          | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
                                          | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
                                          0);
  g_assert_nonnull (ls_green);
  popup = phoc_test_popup_new (globals, 10, 10, 100, 100, 0xFF00FF00, ls_green);
  g_assert_nonnull (popup);

  phoc_test_popup_reposition (globals, popup);

  phoc_test_popup_destroy (popup);
  phoc_test_layer_surface_free (ls_green);

  wl_display_roundtrip (globals->display);

  phoc_assert_screenshot (globals, "empty.png");
  return TRUE;
}


static void
test_layer_shell_popup (void)
{
  PhocTestClientIface iface = { .client_run = test_client_layer_shell_popup };

  phoc_test_client_run (TEST_PHOC_CLIENT_TIMEOUT, &iface, NULL);
}


gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  PHOC_TEST_ADD ("/phoc/layer-shell/anchor", test_layer_shell_anchor);
  PHOC_TEST_ADD ("/phoc/layer-shell/exclusive_zone", test_layer_shell_exclusive_zone);
  PHOC_TEST_ADD ("/phoc/layer-shell/set_layer", test_layer_shell_set_layer);
  PHOC_TEST_ADD ("/phoc/layer-shell/popup", test_layer_shell_popup);

  return g_test_run ();
}
