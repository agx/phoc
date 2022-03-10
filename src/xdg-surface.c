/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-xdg-surface"

#include "config.h"

#include "xdg-surface.h"

G_DEFINE_TYPE (PhocXdgSurface, phoc_xdg_surface, G_TYPE_OBJECT)

static void
phoc_xdg_surface_finalize (GObject *object)
{
  PhocXdgSurface *self = PHOC_XDG_SURFACE(object);

  wl_list_remove(&self->surface_commit.link);
  wl_list_remove(&self->destroy.link);
  wl_list_remove(&self->new_popup.link);
  wl_list_remove(&self->map.link);
  wl_list_remove(&self->unmap.link);
  wl_list_remove(&self->request_move.link);
  wl_list_remove(&self->request_resize.link);
  wl_list_remove(&self->request_maximize.link);
  wl_list_remove(&self->request_fullscreen.link);
  wl_list_remove(&self->set_title.link);
  wl_list_remove(&self->set_app_id.link);
  wl_list_remove(&self->set_parent.link);
  self->xdg_surface->data = NULL;

  G_OBJECT_CLASS (phoc_xdg_surface_parent_class)->finalize (object);
}


static void
phoc_xdg_surface_class_init (PhocXdgSurfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = phoc_xdg_surface_finalize;
}


static void
phoc_xdg_surface_init (PhocXdgSurface *self)
{
}


PhocXdgSurface *
phoc_xdg_surface_new (void)
{
  return PHOC_XDG_SURFACE (g_object_new (PHOC_TYPE_XDG_SURFACE, NULL));
}
