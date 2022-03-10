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

enum {
  PROP_0,
  PROP_WLR_XDG_SURFACE,
  PROP_LAST_PROP
};

static GParamSpec *props[PROP_LAST_PROP];

G_DEFINE_TYPE (PhocXdgSurface, phoc_xdg_surface, G_TYPE_OBJECT)

static void
phoc_xdg_surface_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PhocXdgSurface *self = PHOC_XDG_SURFACE (object);

  switch (property_id) {
  case PROP_WLR_XDG_SURFACE:
    self->xdg_surface = g_value_get_pointer (value);
    self->xdg_surface->data = self;
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

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
  object_class->set_property = phoc_xdg_surface_set_property;

  /**
   * PhocXdgSurface:wlr-xdg-surface:
   *
   * The underlying wlroots xdg-surface
   */
  props[PROP_WLR_XDG_SURFACE] =
    g_param_spec_pointer ("wlr-xdg-surface", "", "",
			  G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_xdg_surface_init (PhocXdgSurface *self)
{
}


PhocXdgSurface *
phoc_xdg_surface_new (struct wlr_xdg_surface *wlr_xdg_surface)
{
  return PHOC_XDG_SURFACE (g_object_new (PHOC_TYPE_XDG_SURFACE,
                                         "wlr-xdg-surface", wlr_xdg_surface,
                                         NULL));
}
