/*
 * Copyright (C) 2024-2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>

G_BEGIN_DECLS

typedef struct _PhocView PhocView;

#define PHOC_TYPE_VIEW_CHILD (phoc_view_child_get_type ())

G_DECLARE_DERIVABLE_TYPE (PhocViewChild, phoc_view_child, PHOC, VIEW_CHILD, GObject)

/**
 * PhocViewChildClass:
 * @parent_class: The object class structure needs to be the first
 *   element in the widget class structure in order for the class mechanism
 *   to work correctly. This allows a PhocViewClass pointer to be cast to
 *   a GObjectClass pointer.
 * @map: Invoked on map. Chain up to parent.
 * @unmap: Invoked on unmap. Chain up to parent.
 * @get_pos: Get the child's position relative to it's parent.
 */
typedef struct _PhocViewChildClass
{
  GObjectClass parent_class;

  void               (*map) (PhocViewChild *self);
  void               (*unmap) (PhocViewChild *self);
  void               (*get_pos) (PhocViewChild *self, int *sx, int *sy);
} PhocViewChildClass;

#define PHOC_TYPE_VIEW_CHILD (phoc_view_child_get_type ())

PhocView *            phoc_view_child_get_view (PhocViewChild *self);
void                  phoc_view_child_apply_damage (PhocViewChild *self);
void                  phoc_view_child_damage_whole (PhocViewChild *self);
void                  phoc_view_child_get_pos (PhocViewChild *self, int *sx, int *sy);
PhocViewChild *       phoc_view_child_get_parent (PhocViewChild *self);
struct wlr_surface *  phoc_view_child_get_wlr_surface (PhocViewChild *self);
void                  phoc_view_child_set_mapped (PhocViewChild *self, bool mapped);

G_END_DECLS
