/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-outputs-states"

#include "phoc-config.h"
#include "outputs-states.h"
#include "settings.h"
#include "utils.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

G_NORETURN static void
print_version (void)
{
  g_print ("Phoc %s - Phone compositor\n", PHOC_VERSION);
  exit (0);
}


static void
list_outputs_states (GHashTable *all_states, gboolean raw)
{
  GHashTableIter iter;
  gpointer key;

  if (!raw) {
    g_print ("Identifiers\n");
    g_print ("-----------\n");
  }

  g_hash_table_iter_init (&iter, all_states);
  while (g_hash_table_iter_next (&iter, &key, NULL)) {
    const char *name = key;
    if (!raw)
      g_print ("'");
    g_print ("%s", name);
    if (!raw)
      g_print ("'");
    g_print ("\n");
  }
}


static gboolean
show_outputs_state (const char *identifier, PhocOutputsStates *outputs_states, gboolean raw)
{
  const char *transform;
  GPtrArray *output_configs;

  output_configs = phoc_outputs_states_lookup (outputs_states, identifier);
  if (!output_configs) {
    g_critical ("Outputs state '%s' not found", identifier);
    return FALSE;
  }

  for (int i = 0; i < output_configs->len; i++) {
    PhocOutputConfig *oc = g_ptr_array_index (output_configs, i);

    g_print ("%s\n", identifier);
    for (int j = 0; j < strlen (identifier); j++)
      g_print ("-");
    g_print ("\n");

    g_print ("      Enabled: %s\n", oc->enable ?  "yes" : "no");

    if (oc->mode.width && oc->mode.height) {
      g_print ("         Mode: %dx%d", oc->mode.width, oc->mode.height);
      if (oc->mode.refresh_rate)
        g_print ("@%.2f", oc->mode.refresh_rate);
      g_print ("\n");
    }

    if (oc->scale)
      g_print ("        Scale: %f\n", oc->scale);

    transform = phoc_utils_transform_to_str (oc->transform);
    g_print ("    Transform: %s\n", transform);

    if (oc->adaptive_sync != PHOC_OUTPUT_ADAPTIVE_SYNC_NONE) {
      const char *enabled;

      enabled = oc->adaptive_sync == PHOC_OUTPUT_ADAPTIVE_SYNC_ENABLED ? "enabled" : "disabled";
      g_print ("Adaptive sync: %s\n", enabled);
    }

    g_print ("\n");
  }

  return TRUE;
}


int
main (int argc, char **argv)
{
  g_autoptr (GOptionContext) opt_context = NULL;
  g_autoptr (GError) err = NULL;
  g_autofree char *db_path = NULL;
  g_autofree char *exec = NULL, *socket = NULL, *show = NULL;
  gboolean version = FALSE, list = FALSE, raw = FALSE;
  g_autoptr (PhocOutputsStates) outputs_states = NULL;
  gboolean success;
  GHashTable *all_states;

  const GOptionEntry options [] = {
    {"db", 'd', 0, G_OPTION_ARG_STRING, &db_path, "Path to the outputs states db", "FILENAME"},
    {"raw", 'r', 0, G_OPTION_ARG_NONE, &raw, "Use raw output", NULL},
    {"list", 0, 0, G_OPTION_ARG_NONE, &list, "List saved outputs states", NULL},
    {"show", 0, 0, G_OPTION_ARG_STRING, &show, "Show saved state for the given identifier", "IDENTIFIER"},
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
  };

  opt_context = g_option_context_new ("- Phoc display config");
  g_option_context_add_main_entries (opt_context, options, NULL);
  if (!g_option_context_parse (opt_context, &argc, &argv, &err)) {
    g_warning ("%s", err->message);
    g_clear_error (&err);
    return 1;
  }

  if (version)
    print_version ();

  setlocale (LC_MESSAGES, "");
  textdomain (GETTEXT_PACKAGE);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

  outputs_states = phoc_outputs_states_new (db_path);
  success = phoc_outputs_states_load (outputs_states, &err);
  if (!success) {
    g_critical ("Failed to parse outputs states: %s", err->message);
    return EXIT_FAILURE;
  }
  all_states = phoc_outputs_states_get_states (outputs_states);

  if (list) {
    list_outputs_states (all_states, raw);
    return EXIT_SUCCESS;
  } else if (show) {
    success = show_outputs_state (show, outputs_states, raw);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  g_critical ("No action given, try --help");
  return EXIT_FAILURE;
}
