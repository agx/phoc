/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

/* Test that wayland clients can connect to the compositor */

#include "testlib.h"

#include <wayland-client-protocol.h>

/* just run the test client, no extra tests */
static void
test_phoc_client_noop (void)
{
  phoc_test_client_run (3, NULL, NULL);
}

static gboolean
create_surface (PhocTestClientGlobals *globals, gpointer data)
{
  g_assert_nonnull(wl_compositor_create_surface (globals->compositor));
  return TRUE;
}

static void
test_phoc_client_surface (void)
{
  PhocTestClientIface iface = { .client_run = create_surface };

  phoc_test_client_run (3, &iface, NULL);
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phoc/client/noop", test_phoc_client_noop);
  g_test_add_func("/phoc/client/surface", test_phoc_client_surface);

  return g_test_run();
}
