/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "output.h"
#include "outputs-states.h"

#include "testlib.h"
#include <wayland-server-protocol.h>

static void
test_phoc_outputs_states_default_path (void)
{
  PhocOutputsStates *outputs_states = phoc_outputs_states_new (NULL);

  g_assert_finalize_object (outputs_states);
}


static void
test_phoc_outputs_states_single_output (void)
{
  g_autoptr (GError) err = NULL;
  PhocOutputsStates *outputs_states = phoc_outputs_states_new ("output-test.gvdb");
  GPtrArray *output_configs;
  PhocOutputConfig *oc;
  gboolean success;

  output_configs = g_ptr_array_new_full (1, (GDestroyNotify) phoc_output_config_destroy);

  oc = phoc_output_config_new ("simple-output-config");
  oc->transform = WL_OUTPUT_TRANSFORM_270;
  oc->scale = 2.75;
  oc->mode.height = 720;
  oc->mode.width = 360;
  oc->mode.refresh_rate = 60000;
  oc->x = 123;
  oc->y = 456;
  oc->adaptive_sync = PHOC_OUTPUT_ADAPTIVE_SYNC_ENABLED;
  g_ptr_array_add (output_configs, oc);

  phoc_outputs_states_update (outputs_states, "simple-output-config", output_configs);
  success = phoc_outputs_states_save (outputs_states, NULL, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  g_assert_finalize_object (outputs_states);

  outputs_states = phoc_outputs_states_new ("output-test.gvdb");
  success = phoc_outputs_states_load (outputs_states, &err);
  g_assert_no_error (err);
  g_assert_true (success);

  output_configs = phoc_outputs_states_lookup (outputs_states, "simple-output-config");
  g_assert_nonnull (output_configs);
  g_assert_cmpint (output_configs->len, ==, 1);
  oc = g_ptr_array_index (output_configs, 0);
  g_assert_cmpstr (oc->name, ==, "simple-output-config");
  g_assert_cmpint (oc->transform, ==, WL_OUTPUT_TRANSFORM_270);
  g_assert_cmpfloat (oc->scale, ==, 2.75);
  g_assert_cmpint (oc->mode.height, ==, 720);
  g_assert_cmpint (oc->mode.width, ==, 360);
  g_assert_cmpfloat (oc->mode.refresh_rate, ==, 60000);
  g_assert_cmpint (oc->x, ==, 123);
  g_assert_cmpint (oc->y, ==, 456);
  g_assert_cmpint (oc->adaptive_sync, ==, PHOC_OUTPUT_ADAPTIVE_SYNC_ENABLED);

  g_assert_finalize_object (outputs_states);
}


gint
main (gint argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/phoc/output-states/default-path", test_phoc_outputs_states_default_path);
  g_test_add_func ("/phoc/output-states/single-output", test_phoc_outputs_states_single_output);

  return g_test_run ();
}
