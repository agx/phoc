/*
 * Copyright (C) 2020 Purism SPC
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "server.h"

static void
test_phoc_server_get_default (void)
{
  PhocServer *server = phoc_server_get_default ();
  PhocServer *server2;

  g_assert_true (PHOC_IS_SERVER (server));

  server2 = phoc_server_get_default ();
  g_assert_true (server2 == server);
}

static void
test_phoc_server_setup (void)
{
  PhocServer *server = phoc_server_get_default ();

  g_assert_true (PHOC_IS_SERVER (server));

  g_assert_true (phoc_server_setup(server, NULL, NULL, false));
}

static void
test_phoc_server_setup_args (void)
{
  PhocServer *server = phoc_server_get_default ();

  g_assert_true (PHOC_IS_SERVER (server));

  g_assert_true (phoc_server_setup(server, NULL, "/bin/bash", TRUE));

  g_assert_cmpstr (server->config->startup_cmd, ==, "/bin/bash");
  g_assert_true (server->config->debug_damage_tracking);
}

gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/phoc/server/get_default", test_phoc_server_get_default);
  g_test_add_func("/phoc/server/setup", test_phoc_server_setup);
  g_test_add_func("/phoc/server/setup-args", test_phoc_server_setup_args);

  return g_test_run();
}
