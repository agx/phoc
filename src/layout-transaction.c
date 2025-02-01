/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-layout-transaction"

#include "phoc-config.h"
#include "desktop.h"
#include "output.h"
#include "server.h"
#include "layer-surface.h"

#include "layout-transaction.h"

/**
 * PhocLayoutTransaction:
 *
 * Track configures from layer surfaces and emit a signal when all of
 * them have committed new matching buffers.
 */

#define TIMEOUT_MS 250

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocLayoutTransaction {
  GObject               parent;

  gint64                starttime;
  guint                 pending_configures;
  guint                 timer_id;
};
G_DEFINE_TYPE (PhocLayoutTransaction, phoc_layout_transaction, G_TYPE_OBJECT)


static void
apply_transaction (PhocLayoutTransaction *self)
{
  g_debug ("Applying layout transaction");
  /* TODO: Drop all saved buffers so rendering picks up the committed ones */
}


static void
on_timeout_expired (gpointer user_data)
{
  PhocLayoutTransaction *self = PHOC_LAYOUT_TRANSACTION (user_data);

  self->timer_id = 0;
  g_warning ("Timeout (%dms) expired with %u configures pending",
             TIMEOUT_MS, self->pending_configures);
  apply_transaction (self);
}


static void
phoc_layout_transaction_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  PhocLayoutTransaction *self = PHOC_LAYOUT_TRANSACTION (object);

  switch (property_id) {
  case PROP_ACTIVE:
    g_value_set_boolean (value, phoc_layout_transaction_is_active (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_layout_transaction_dispose (GObject *object)
{
  PhocLayoutTransaction *self = PHOC_LAYOUT_TRANSACTION (object);

  g_clear_handle_id (&self->timer_id, g_source_remove);

  G_OBJECT_CLASS (phoc_layout_transaction_parent_class)->dispose (object);
}


static void
phoc_layout_transaction_finalize (GObject *object)
{
  PhocLayoutTransaction *self = PHOC_LAYOUT_TRANSACTION (object);

  apply_transaction (self);

  G_OBJECT_CLASS (phoc_layout_transaction_parent_class)->finalize (object);
}


static void
phoc_layout_transaction_class_init (PhocLayoutTransactionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_layout_transaction_get_property;
  object_class->dispose = phoc_layout_transaction_dispose;
  object_class->finalize = phoc_layout_transaction_finalize;

  props[PROP_ACTIVE] =
    g_param_spec_boolean ("active", "", "",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_layout_transaction_init (PhocLayoutTransaction *self)
{
}

/**
 * phoc_layout_transaction_get_default:
 *
 * Get the layout transaction singleton
 *
 * Returns: (transfer none):The layout transaction singleton
 */
PhocLayoutTransaction *
phoc_layout_transaction_get_default (void)
{
  static PhocLayoutTransaction *instance;

  if (G_UNLIKELY (instance == NULL)) {
    g_debug ("Creating layout transaction singleton");
    instance = g_object_new (PHOC_TYPE_LAYOUT_TRANSACTION, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *)&instance);
  }
  return instance;
}


gboolean
phoc_layout_transaction_is_active (PhocLayoutTransaction *self)
{
  g_assert (PHOC_IS_LAYOUT_TRANSACTION (self));

  return self->pending_configures > 0;
}

/**
 * phoc_layout_transaction_add_dirty:
 * @self: The transaction
 *
 * Invoked by views / layer surfaces when they want to become part of
 * a layout transaction. If the number of outstanding configures isn't
 * 0 we consider the transaction dirty (in progress).
 */
void
phoc_layout_transaction_add_dirty (PhocLayoutTransaction *self)
{
  guint pending;

  g_assert (PHOC_IS_LAYOUT_TRANSACTION (self));

  pending = self->pending_configures;
  self->pending_configures++;
  if (pending) {
    g_debug ("Layout transaction adding %dth pending configure", self->pending_configures);
    self->starttime = g_get_monotonic_time ();
    return;
  }

  /* Outstanding configures. Transaction started */
  g_debug ("Starting new layout transaction");
  self->timer_id = g_timeout_add_once (TIMEOUT_MS, on_timeout_expired, self);
  g_source_set_name_by_id (self->timer_id, "[phoc] layout transaction timer");
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIVE]);
}

/**
 * phoc_layout_transaction_notify_configured:
 * @self: The transaction
 *
 * Invoked by views / layer surfaces when they handle a commit and
 * they were part of a transaction. If the number of outstanding
 * configures drops to 0 we apply the transaction.
 */
void
phoc_layout_transaction_notify_configured (PhocLayoutTransaction *self)
{
  gint64 now;

  g_assert (PHOC_IS_LAYOUT_TRANSACTION (self));

  g_return_if_fail (self->pending_configures > 0);

  self->pending_configures--;
  if (self->pending_configures) {
    g_debug ("Layout transaction has %u configures pending", self->pending_configures);
    return;
  }

  /* All outstanding configures committed buffers */
  now = g_get_monotonic_time ();
  g_debug ("Layout transaction finished after %" G_GINT64_FORMAT "ms", (now - self->starttime) / 1000);
  g_clear_handle_id (&self->timer_id, g_source_remove);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIVE]);

  apply_transaction (self);
}
