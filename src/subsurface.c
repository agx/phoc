/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-subsurface"

#include "phoc-config.h"

#include "subsurface.h"

G_DEFINE_FINAL_TYPE (PhocSubsurface, phoc_subsurface, PHOC_TYPE_VIEW_CHILD)

static void
subsurface_get_pos (PhocViewChild *child, int *sx, int *sy)
{
  struct wlr_surface *wlr_surface;
  struct wlr_subsurface *wlr_subsurface;

  wlr_surface = child->wlr_surface;
  if (child->parent)
    phoc_view_child_get_pos (child->parent, sx, sy);
  else
    *sx = *sy = 0;

  wlr_subsurface = wlr_subsurface_try_from_wlr_surface (wlr_surface);
  g_assert (wlr_subsurface);
  *sx += wlr_subsurface->current.x;
  *sy += wlr_subsurface->current.y;
}


static void
phoc_subsurface_finalize (GObject *object)
{
  PhocSubsurface *self = PHOC_SUBSURFACE (object);

  wl_list_remove (&self->destroy.link);

  G_OBJECT_CLASS (phoc_subsurface_parent_class)->finalize (object);
}


static void
phoc_subsurface_class_init (PhocSubsurfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  PhocViewChildClass *view_child_class = PHOC_VIEW_CHILD_CLASS (klass);

  object_class->finalize = phoc_subsurface_finalize;
  view_child_class->get_pos = subsurface_get_pos;
}


static void
phoc_subsurface_init (PhocSubsurface *self)
{
}


PhocSubsurface *
phoc_subsurface_new (PhocView *view, struct wlr_surface *wlr_surface)
{
  return g_object_new (PHOC_TYPE_SUBSURFACE,
                       "view", view,
                       "wlr-surface", wlr_surface,
                       NULL);
}
