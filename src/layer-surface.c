/*
 * Copyright (C) 2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0+
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-layer-surface"

#include "config.h"

#include "layer-surface.h"

G_DEFINE_TYPE (PhocLayerSurface, phoc_layer_surface, G_TYPE_OBJECT)


static void
phoc_layer_surface_class_init (PhocLayerSurfaceClass *klass)
{
}


static void
phoc_layer_surface_init (PhocLayerSurface *self)
{
}


PhocLayerSurface *
phoc_layer_surface_new (void)
{
  return PHOC_LAYER_SURFACE (g_object_new (PHOC_TYPE_LAYER_SURFACE, NULL));
}
