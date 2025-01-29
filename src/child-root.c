/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-child-root"

#include "phoc-config.h"

#include "child-root.h"

/**
 * PhocChildRoot:
 *
 * Implementations of the `PhocChildRoot` interface. Implemented by `PhocView`
 * and `PhocLayerSurface` so both surfaces roles can use the same handling for
 * subsurfaces and popups.
 */

G_DEFINE_INTERFACE (PhocChildRoot, phoc_child_root, G_TYPE_OBJECT)

void
phoc_child_root_default_init (PhocChildRootInterface *iface)
{
}


void
phoc_child_root_get_box (PhocChildRoot *self, struct wlr_box *box)
{
  PhocChildRootInterface *iface;

  g_assert (PHOC_IS_CHILD_ROOT (self));
  iface = PHOC_CHILD_ROOT_GET_IFACE (self);

  iface->get_box (self, box);
}


gboolean
phoc_child_root_is_mapped (PhocChildRoot *self)
{
  PhocChildRootInterface *iface;

  g_assert (PHOC_IS_CHILD_ROOT (self));
  iface = PHOC_CHILD_ROOT_GET_IFACE (self);

  return iface->is_mapped (self);
}


void
phoc_child_root_apply_damage (PhocChildRoot *self)
{
  PhocChildRootInterface *iface;

  g_assert (PHOC_IS_CHILD_ROOT (self));
  iface = PHOC_CHILD_ROOT_GET_IFACE (self);

  iface->apply_damage (self);
}


void
phoc_child_root_add_child (PhocChildRoot *self, PhocViewChild  *child)
{
  PhocChildRootInterface *iface;

  g_assert (PHOC_IS_CHILD_ROOT (self));
  iface = PHOC_CHILD_ROOT_GET_IFACE (self);

  iface->add_child (self, child);
}


void
phoc_child_root_remove_child (PhocChildRoot *self, PhocViewChild  *child)
{
  PhocChildRootInterface *iface;

  g_assert (PHOC_IS_CHILD_ROOT (self));
  iface = PHOC_CHILD_ROOT_GET_IFACE (self);

  iface->remove_child (self, child);
}


gboolean
phoc_child_root_unconstrain_popup (PhocChildRoot *self, struct wlr_box *box)
{
  PhocChildRootInterface *iface;

  g_assert (PHOC_IS_CHILD_ROOT (self));
  iface = PHOC_CHILD_ROOT_GET_IFACE (self);

  return iface->unconstrain_popup (self, box);
}
