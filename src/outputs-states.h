/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define PHOC_TYPE_OUTPUTS_STATES (phoc_outputs_states_get_type ())

G_DECLARE_FINAL_TYPE (PhocOutputsStates, phoc_outputs_states, PHOC, OUTPUTS_STATES, GObject)

PhocOutputsStates *phoc_outputs_states_new (const char *state_file);
void               phoc_outputs_states_update (PhocOutputsStates *self,
                                               const char        *identifier,
                                               GPtrArray         *output_configs);
GPtrArray *        phoc_outputs_states_lookup (PhocOutputsStates *self, const char *identifier);
void               phoc_outputs_states_save_async (PhocOutputsStates    *self,
                                                   GAsyncReadyCallback   callback,
                                                   GCancellable         *cancellable,
                                                   gpointer              user_data);
gboolean           phoc_outputs_states_save_finish (PhocOutputsStates *self,
                                                    GAsyncResult      *res,
                                                    GError           **error);
gboolean           phoc_outputs_states_save (PhocOutputsStates *self,
                                             GCancellable      *cancellable,
                                             GError           **error);
gboolean           phoc_outputs_states_load (PhocOutputsStates *self, GError **err);
GHashTable *       phoc_outputs_states_get_states (PhocOutputsStates *self);

G_END_DECLS
