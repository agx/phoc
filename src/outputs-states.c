/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-outputs-states"

#include "phoc-config.h"

#include "outputs-states.h"
#include "settings.h"

#include <gmobile.h>
#include <gvdb/gvdb-builder.h>
#include <gvdb/gvdb-reader.h>

#define STATE_FORMAT_VERSION 1

/**
 * PhocOutputsStates:
 *
 * An "outputs state" is the current state of all outputs. The
 * `PhocOutputsStates` class tracks these and persists them. Consumers can
 * pass in an `identifier` to lookup up a specific outputs state. The identifier
 * is handled by the caller and should be unique for one set of outputs.
 */

enum {
  PROP_0,
  PROP_STATE_FILE,
  PROP_CURRENT_STATE_ID,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocOutputsStates {
  GObject       parent;

  char         *state_file;
  char         *current_state_id;
  GHashTable   *outputs_states; /* key: identifier, value: array of PhocOutputConfig */
};
G_DEFINE_TYPE (PhocOutputsStates, phoc_outputs_states, G_TYPE_OBJECT)


typedef struct {
  GAsyncResult *res;
  GMainLoop    *loop;
} PhocOutputsStatesSyncData;


static void
phoc_outputs_states_set_state_file (PhocOutputsStates *self, const char *state_file)
{
  g_set_str (&self->state_file, state_file);
  if (self->state_file)
    return;

  self->state_file = g_build_path ("/", g_get_user_state_dir (), "phoc", "outputs.gvdb", NULL);
}


static void
phoc_outputs_states_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  PhocOutputsStates *self = PHOC_OUTPUTS_STATES (object);

  switch (property_id) {
  case PROP_STATE_FILE:
    phoc_outputs_states_set_state_file (self, g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_outputs_states_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  PhocOutputsStates *self = PHOC_OUTPUTS_STATES (object);

  switch (property_id) {
  case PROP_CURRENT_STATE_ID:
    g_value_set_string (value, self->current_state_id);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_outputs_states_dispose (GObject *object)
{
  PhocOutputsStates *self = PHOC_OUTPUTS_STATES (object);

  g_clear_pointer (&self->outputs_states, g_hash_table_unref);
  g_clear_pointer (&self->state_file, g_free);

  G_OBJECT_CLASS (phoc_outputs_states_parent_class)->dispose (object);
}


static void
phoc_outputs_states_class_init (PhocOutputsStatesClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_outputs_states_get_property;
  object_class->set_property = phoc_outputs_states_set_property;
  object_class->dispose = phoc_outputs_states_dispose;

  /**
   * PhocOutputsStatess:state-file:
   *
   * Output file name
   */
  props[PROP_STATE_FILE] =
    g_param_spec_string ("state-file", "", "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);
  /**
   * PhocOutputsStatess:current-state-id:
   *
   * Identifier of the current output configuration state
   */
  props[PROP_CURRENT_STATE_ID] =
    g_param_spec_string ("current-state-id", "", "",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_outputs_states_init (PhocOutputsStates *self)
{
  self->outputs_states = g_hash_table_new_full (g_str_hash,
                                                g_str_equal,
                                                g_free,
                                                (GDestroyNotify)g_ptr_array_unref);
}


PhocOutputsStates *
phoc_outputs_states_new (const char *state_file)
{
  return g_object_new (PHOC_TYPE_OUTPUTS_STATES,
                       "state-file", state_file,
                       NULL);
}

/**
 * phoc_outputs_states_serialize:
 * @outputs_states: The output states
 * @gvdb_data: The hash table to serialize to
 *
 * Serialize a single output configuration state in a way suitable for gvdb
 */
static void
phoc_outputs_states_serialize (GPtrArray *outputs_states, GHashTable *gvdb_data)
{
  g_autoptr (GHashTable) outputs = NULL;
  GvdbItem *item;

  item = gvdb_hash_table_insert (gvdb_data, "version");
  gvdb_item_set_value (item, g_variant_new ("i", STATE_FORMAT_VERSION));

  outputs = gvdb_hash_table_new (gvdb_data, "outputs");

  for (int i = 0; i < outputs_states->len; i++) {
    PhocOutputConfig *oc = g_ptr_array_index (outputs_states, i);
    g_autoptr (GHashTable) output = NULL;

    g_debug ("Serializing output '%s'", oc->name);
    output = gvdb_hash_table_new (outputs, oc->name);

    item = gvdb_hash_table_insert (output, "enabled");
    gvdb_item_set_value (item, g_variant_new_boolean (oc->enable));

    item = gvdb_hash_table_insert (output, "transform");
    gvdb_item_set_value (item, g_variant_new ("u", oc->transform));

    if (oc->scale) {
      item = gvdb_hash_table_insert (output, "scale");
      gvdb_item_set_value (item, g_variant_new ("d", oc->scale));
    }

    if (oc->mode.width && oc->mode.height && oc->mode.refresh_rate) {
      item = gvdb_hash_table_insert (output, "mode");
      gvdb_item_set_value (item, g_variant_new ("(iid)",
                                                oc->mode.width,
                                                oc->mode.height,
                                                (double)oc->mode.refresh_rate));
    }

    if (oc->x != -1 && oc->y != -1) {
      item = gvdb_hash_table_insert (output, "layout-position");
      gvdb_item_set_value (item, g_variant_new ("(ii)", oc->x,  oc->y));
    }
  }
}


static void
on_save_outputs_states_ready (GObject *object, GAsyncResult *res, gpointer data)
{
  g_autoptr (GTask) task = G_TASK (data);
  GHashTable *gvdb_table;
  GError *err = NULL;
  gboolean success;

  g_assert (G_IS_TASK (task));
  gvdb_table = g_task_get_task_data (task);
  g_assert (gvdb_table);

  success = gvdb_table_write_contents_finish (gvdb_table, res, &err);
  if (!success) {
    g_task_return_error (task, err);
    return;
  }

  g_task_return_boolean (task, TRUE);
}


void
phoc_outputs_states_save_async (PhocOutputsStates    *self,
                                GAsyncReadyCallback   callback,
                                GCancellable         *cancellable,
                                gpointer              user_data)
{
  g_autoptr (GTask) task = g_task_new (self, cancellable, callback, user_data);
  g_autofree char *statedir = NULL;
  GHashTableIter iter;
  GHashTable *gvdb_table;
  GPtrArray *outputs_states;
  char *state_id;
  int ret;

  g_assert (PHOC_IS_OUTPUTS_STATES (self));

  g_task_set_source_tag (task, phoc_outputs_states_save_async);
  statedir = g_path_get_dirname (self->state_file);
  ret = g_mkdir_with_parents (statedir, 0755);
  if (ret != 0) {
    g_task_return_error (task, g_error_new (G_IO_ERROR,
                                            g_io_error_from_errno (ret),
                                            "Failed to create state directory"));
    return;
  }

  gvdb_table = gvdb_hash_table_new (NULL, NULL);
  g_hash_table_iter_init (&iter, self->outputs_states);
  while (g_hash_table_iter_next (&iter, (gpointer*) &state_id, (gpointer*) &outputs_states)) {
    g_autoptr (GHashTable) outputs_state_table = NULL;

    outputs_state_table = gvdb_hash_table_new (gvdb_table, state_id);
    phoc_outputs_states_serialize (outputs_states, outputs_state_table);
  }

  g_task_set_task_data (task, gvdb_table, (GDestroyNotify) g_hash_table_unref);

  gvdb_table_write_contents_async (gvdb_table,
                                   self->state_file,
                                   FALSE,
                                   cancellable,
                                   on_save_outputs_states_ready,
                                   g_steal_pointer (&task));
}


gboolean
phoc_outputs_states_save_finish (PhocOutputsStates *self,
                                 GAsyncResult      *res,
                                 GError           **error)
{
  g_assert (PHOC_IS_OUTPUTS_STATES (self));
  g_assert (G_IS_TASK (res));
  g_assert (!error || !*error);
  g_assert (g_task_get_source_tag (G_TASK (res)) == phoc_outputs_states_save_async);

  return g_task_propagate_boolean (G_TASK (res), error);
}


static void
on_phoc_outputs_states_save_ready (GObject      *object,
                                   GAsyncResult *res,
                                   gpointer      user_data)
{
  PhocOutputsStatesSyncData *data = user_data;
  data->res = g_object_ref (res);

  g_main_loop_quit (data->loop);
}


gboolean
phoc_outputs_states_save (PhocOutputsStates *self,
                          GCancellable      *cancellable,
                          GError           **error)
{
  gboolean success;
  g_autoptr (GMainContext) context = g_main_context_new ();
  g_autoptr (GMainLoop) loop = NULL;
  PhocOutputsStatesSyncData data;

  g_main_context_push_thread_default (context);
  loop = g_main_loop_new (context, FALSE);

  data = (PhocOutputsStatesSyncData) {
    .loop = loop,
    .res = NULL,
  };

  phoc_outputs_states_save_async (self,
                                  on_phoc_outputs_states_save_ready,
                                  cancellable,
                                  &data);
  g_main_loop_run (data.loop);

  success = phoc_outputs_states_save_finish (self, data.res, error);

  g_clear_object (&data.res);
  g_main_context_pop_thread_default (context);

  return success;
}

/**
 * phoc_outputs_states_deserialize:
 * @gvdb_data: The hash table to serialize to
 *
 * Deserialize a single output configuration from the given data
 */
static GPtrArray *
phoc_outputs_states_deserialize (GvdbTable *gvdb_table, GError **err)
{
  g_autoptr (GPtrArray) output_configs = NULL;
  g_auto (GStrv) output_table_names = NULL;
  g_autoptr (GVariant) version = NULL;
  g_autoptr (GvdbTable) outputs_table = NULL;

  output_configs = g_ptr_array_new_full (1, (GDestroyNotify) phoc_output_config_destroy);

  version = gvdb_table_get_value (gvdb_table, "version");
  if (!g_variant_is_of_type (version, G_VARIANT_TYPE ("i")) ||
      g_variant_get_int32 (version) != STATE_FORMAT_VERSION) {
    g_set_error (err,
                 G_IO_ERROR,
                 G_IO_ERROR_FAILED,
                 "Unparsable output configuration version");
    return FALSE;
  }

  outputs_table = gvdb_table_get_table (gvdb_table, "outputs");
  if (!outputs_table) {
    g_set_error (err,
                 G_IO_ERROR,
                 G_IO_ERROR_INVALID_ARGUMENT,
                 "Output list must not be empty");
  }

  output_table_names = gvdb_table_get_names (outputs_table, NULL);
  for (int i = 0; output_table_names[i]; i++) {
    const char *output_name = output_table_names[i];
    g_autoptr (GvdbTable) output_table = gvdb_table_get_table (outputs_table, output_name);
    g_autoptr (GVariant) transform = NULL, scale = NULL, mode = NULL, layout_pos = NULL;
    g_autoptr (GVariant) enabled = NULL;
    g_autoptr (PhocOutputConfig) oc = phoc_output_config_new (output_name);

    g_debug ("Deserializing output state '%s'", output_name);

    enabled = gvdb_table_get_value (output_table, "enabled");
    if (enabled && g_variant_is_of_type (enabled, G_VARIANT_TYPE ("b")))
      oc->enable = g_variant_get_boolean (enabled);

    transform = gvdb_table_get_value (output_table, "transform");
    if (transform && g_variant_is_of_type (transform, G_VARIANT_TYPE ("u")))
      oc->transform = g_variant_get_uint32 (transform);

    scale = gvdb_table_get_value (output_table, "scale");
    if (scale && g_variant_is_of_type (scale, G_VARIANT_TYPE ("d")))
      oc->scale = g_variant_get_double (scale);

    mode = gvdb_table_get_value (output_table, "mode");
    if (mode && g_variant_is_of_type (mode, G_VARIANT_TYPE ("(iid)"))) {
      double refresh_rate;

      g_variant_get (mode, "(iid)",
                     &oc->mode.width,
                     &oc->mode.height,
                     &refresh_rate);
      oc->mode.refresh_rate = (float)refresh_rate;
    }

    layout_pos = gvdb_table_get_value (output_table, "layout-position");
    if (layout_pos && g_variant_is_of_type (layout_pos, G_VARIANT_TYPE ("(ii)")))
      g_variant_get (layout_pos, "(ii)", &oc->x, &oc->y);

    g_ptr_array_add (output_configs, g_steal_pointer (&oc));
  }

  return g_steal_pointer (&output_configs);
}


gboolean
phoc_outputs_states_load (PhocOutputsStates *self, GError **err)
{
  g_autoptr (GError) local_err = NULL;
  g_autoptr (GBytes) bytes = NULL;
  g_autoptr (GvdbTable) gvdb_table = NULL;
  g_autoptr (GMappedFile) mapped_file = NULL;
  g_auto (GStrv) names = NULL;

  mapped_file = g_mapped_file_new (self->state_file, FALSE, &local_err);
  if (!mapped_file) {
    if (g_error_matches (local_err, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
      return TRUE;

    g_propagate_error (err, local_err);
    return FALSE;
  }

  if (g_mapped_file_get_length (mapped_file) == 0)
    return TRUE;

  bytes = g_mapped_file_get_bytes (mapped_file);
  gvdb_table = gvdb_table_new_from_bytes (bytes, FALSE, err);

  names = gvdb_table_get_names (gvdb_table, NULL);
  if (gm_strv_is_null_or_empty (names))
    return TRUE;

  for (int i = 0; names[i]; i++) {
    g_autoptr (GPtrArray) output_configs = NULL;
    g_autoptr (GvdbTable) state_table = NULL;
    const char *identifier = names[i];

    g_debug ("Deserializing output states for '%s'", identifier);
    state_table = gvdb_table_get_table (gvdb_table, identifier);
    g_assert (state_table);

    output_configs = phoc_outputs_states_deserialize (state_table, &local_err);
    if (!output_configs) {
      if (g_error_matches (local_err, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT)) {
        g_warning ("Skipping broken config '%s'", identifier);
        continue;
      } else {
        g_propagate_error (err, local_err);
        return FALSE;
      }
    }

    g_hash_table_insert (self->outputs_states,
                         g_strdup (identifier),
                         g_steal_pointer (&output_configs));
  }

  return TRUE;
}

/**
 * phoc_outputs_states_update:
 * @self: The output states handler
 * @identifier: The identifier we want to update the outputs state for
 * @output_configs: (transfer full)(element-type PhocOutputConfig): The new output config
 *
 * Add the given configuration to the list of known output states.
 */
void
phoc_outputs_states_update (PhocOutputsStates *self,
                            const char        *identifier,
                            GPtrArray         *output_configs)
{
  g_assert (PHOC_IS_OUTPUTS_STATES (self));
  g_assert (output_configs);

  g_debug ("Updating output config for '%s'", identifier);

  g_hash_table_insert (self->outputs_states,
                       g_strdup (identifier),
                       g_steal_pointer (&output_configs));
}

/**
 * phoc_outputs_states_lookup:
 * @self: The output states handler
 * @identifier: The identifier we want to look up the outputs state for
 *
 * Given an identifier look up the corresponding outputs state
 *
 * Returns: (element-type PhocOutputConfig)(transfer full): The new output config
 */
GPtrArray *
phoc_outputs_states_lookup (PhocOutputsStates *self, const char *identifier)
{
  g_assert (PHOC_IS_OUTPUTS_STATES (self));

  return g_hash_table_lookup (self->outputs_states, identifier);
}

/**
 * phoc_outputs_states_get_states:
 * @self: The outputs states
 *
 * Get all known outputs states
 *
 * Returns: (transfer none): The outputs states)
 */
GHashTable *
phoc_outputs_states_get_states (PhocOutputsStates *self)
{
  g_assert (PHOC_IS_OUTPUTS_STATES (self));

  return self->outputs_states;
}
