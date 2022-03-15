/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-xwayland-surface"

#include "config.h"

#include "xwayland-surface.h"

#include <wlr/xwayland.h>

enum {
  PROP_0,
  PROP_WLR_XWAYLAND_SURFACE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

G_DEFINE_TYPE (PhocXWaylandSurface, phoc_xwayland_surface, G_TYPE_OBJECT)

static void
phoc_xwayland_surface_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PhocXWaylandSurface *self = PHOC_XWAYLAND_SURFACE (object);

  switch (property_id) {
  case PROP_WLR_XWAYLAND_SURFACE:
    self->xwayland_surface = g_value_get_pointer (value);
    self->xwayland_surface->data = self;
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_xwayland_surface_constructed (GObject *object)
{
  PhocXWaylandSurface *self = PHOC_XWAYLAND_SURFACE(object);

  G_OBJECT_CLASS (phoc_xwayland_surface_parent_class)->constructed (object);
}


static void
phoc_xwayland_surface_finalize (GObject *object)
{
  PhocXWaylandSurface *self = PHOC_XWAYLAND_SURFACE(object);

  self->xwayland_surface->data = NULL;

  G_OBJECT_CLASS (phoc_xwayland_surface_parent_class)->finalize (object);
}


static void
phoc_xwayland_surface_class_init (PhocXWaylandSurfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = phoc_xwayland_surface_set_property;
  object_class->constructed = phoc_xwayland_surface_constructed;
  object_class->finalize = phoc_xwayland_surface_finalize;

  /**
   * PhocXWaylandSurface:wlr-xwayland-surface:
   *
   * The underlying wlroots xwayland-surface
   */
  props[PROP_WLR_XWAYLAND_SURFACE] =
    g_param_spec_pointer ("wlr-xwayland-surface", "", "",
			  G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_xwayland_surface_init (PhocXWaylandSurface *self)
{
}


PhocXWaylandSurface *
phoc_xwayland_surface_new (struct wlr_xwayland_surface *surface)
{
  return PHOC_XWAYLAND_SURFACE (g_object_new (PHOC_TYPE_XWAYLAND_SURFACE,
                                              "wlr-xwayland-surface", surface,
                                              NULL));
}
