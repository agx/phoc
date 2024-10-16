/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_LAYOUT_TRANSACTION (phoc_layout_transaction_get_type ())

G_DECLARE_FINAL_TYPE (PhocLayoutTransaction, phoc_layout_transaction, PHOC, LAYOUT_TRANSACTION, GObject)

PhocLayoutTransaction *phoc_layout_transaction_get_default (void);
gboolean               phoc_layout_transaction_is_active (PhocLayoutTransaction *self);
void                   phoc_layout_transaction_notify_configured (PhocLayoutTransaction *self);
void                   phoc_layout_transaction_add_dirty (PhocLayoutTransaction *self);

G_END_DECLS
