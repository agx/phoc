/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "view-child-private.h"

#include <glib-object.h>

#include <wlr/util/box.h>

G_BEGIN_DECLS

typedef struct _PhocViewChild PhocViewChild;

#define PHOC_TYPE_CHILD_ROOT (phoc_child_root_get_type ())
G_DECLARE_INTERFACE (PhocChildRoot, phoc_child_root, PHOC, CHILD_ROOT, GObject)

/**
 * PhocChildRootInterface:
 * @parent_iface: The parent interface
 * @get_box: Get the root's surface box
 * @is_mapped: Check whether the root is mapped
 * @apply_damage: Submit the accumulated damage for the root and its children
 * @add_child: Invoked when a new child should is added
 * @remove_child: Invoked when a child should no longer be tracked by the root.
 * @unconstrain_popup: Get a box that the popup can use to unconstrain itself.
 *   Coordinates are relative to the toplevel's top, left corner.
 *
 * The list of virtual functions for the `PhocChildRoot` interface. Interfaces
 * are required to implement all virtual functions.
 */

struct _PhocChildRootInterface
{
  GTypeInterface parent_iface;

  void         (*get_box)           (PhocChildRoot *root, struct wlr_box *box);
  gboolean     (*is_mapped)         (PhocChildRoot *root);
  void         (*apply_damage)      (PhocChildRoot *root);
  void         (*add_child)         (PhocChildRoot *root, PhocViewChild *child);
  void         (*remove_child)      (PhocChildRoot *root, PhocViewChild *child);
  gboolean     (*unconstrain_popup) (PhocChildRoot *root, struct wlr_box *box);
};

void                    phoc_child_root_get_box                 (PhocChildRoot  *self,
                                                                 struct wlr_box *box);
gboolean                phoc_child_root_is_mapped               (PhocChildRoot  *self);
void                    phoc_child_root_apply_damage            (PhocChildRoot  *self);
void                    phoc_child_root_add_child               (PhocChildRoot  *self,
                                                                 PhocViewChild  *child);
void                    phoc_child_root_remove_child            (PhocChildRoot  *self,
                                                                 PhocViewChild  *child);
gboolean                phoc_child_root_unconstrain_popup       (PhocChildRoot  *self,
                                                                 struct wlr_box *box);
G_END_DECLS
