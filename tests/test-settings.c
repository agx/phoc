/*
 * Copyright (C) 2023 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "settings.h"

#include "testlib.h"


static void
test_phoc_config_defaults (void)
{
  g_autoptr (PhocConfig) config = phoc_config_new_from_data ("");

  g_assert_true (config->xwayland);
  g_assert_true (config->xwayland_lazy);
  g_assert_cmpint (g_slist_length (config->outputs), ==, 0);
  g_assert_null (config->config_path);
}


static void
test_phoc_config_output (void)
{
  PhocOutputConfig *oc;

  g_autoptr (PhocConfig) config1 = phoc_config_new_from_data (
    "[output:X11-1]\n"
    "scale = 3\n"
    "adaptive-sync = enabled\n");

  g_autoptr (PhocConfig) config2 = phoc_config_new_from_data (
    "[output:X11-1]\n"
    "scale = 3\n"
    "[output:X11-2]\n"
    "scale = 3\n");

  oc = config1->outputs->data;
  g_assert_cmpint (g_slist_length (config1->outputs), ==, 1);
  g_assert_cmpfloat (oc->scale, ==, 3.0);
  g_assert_cmpint (oc->adaptive_sync, ==, PHOC_OUTPUT_ADAPTIVE_SYNC_ENABLED);
  g_assert_cmpint (g_slist_length (config1->outputs), ==, 1);


  g_assert_cmpint (g_slist_length (config2->outputs), ==, 2);
}


static void
test_phoc_config_modelines (void)
{
  PhocOutputConfig *oc;

  g_autoptr (PhocConfig) config = phoc_config_new_from_data (
    "[output:X11-1]\n"
    "modeline = 87.25  720 776 848 976  1440 1443 1453 1493 -hsync +vsync\n"
    "modeline = 87.25  720 776 848 976  1440 1443 1453 1493 -hsync +vsync\n"
    "scale = 3\n");

  oc = config->outputs->data;
  g_assert_cmpint (g_slist_length (config->outputs), ==, 1);
  g_assert_cmpfloat (oc->scale, ==, 3.0);
  g_assert_cmpint (oc->adaptive_sync, ==, PHOC_OUTPUT_ADAPTIVE_SYNC_NONE);
  g_assert_cmpint (g_slist_length (oc->modes), ==, 2);
}


gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phoc/config/simple", test_phoc_config_defaults);
  g_test_add_func ("/phoc/config/output", test_phoc_config_output);
  g_test_add_func ("/phoc/config/modelines", test_phoc_config_modelines);

  return g_test_run ();
}
