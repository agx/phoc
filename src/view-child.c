/*
 * Copyright (C) 2024 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-view-child"

#include "phoc-config.h"

#include "view.h"
#include "view-child-private.h"


enum {
  PROP_0,
  PROP_VIEW,
  PROP_WLR_SURFACE,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];


G_DEFINE_TYPE (PhocViewChild, phoc_view_child, G_TYPE_OBJECT)


static void
phoc_view_child_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  PhocViewChild *self = PHOC_VIEW_CHILD (object);

  switch (property_id) {
  case PROP_VIEW:
    /* TODO: Should hold a ref */
    self->view = g_value_get_object (value);
    break;
  case PROP_WLR_SURFACE:
    self->wlr_surface = g_value_get_pointer (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_view_child_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  PhocViewChild *self = PHOC_VIEW_CHILD (object);

  switch (property_id) {
  case PROP_VIEW:
    g_value_set_object (value, self->view);
    break;
  case PROP_WLR_SURFACE:
    g_value_set_pointer (value, self->wlr_surface);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_view_child_constructed (GObject *object)
{
  G_OBJECT_CLASS (phoc_view_child_parent_class)->constructed (object);
}


static void
phoc_view_child_finalize (GObject *object)
{
  PhocViewChild *self = PHOC_VIEW_CHILD (object);

  self->view = NULL;
  self->wlr_surface = NULL;

  G_OBJECT_CLASS (phoc_view_child_parent_class)->finalize (object);
}


static void
phoc_view_child_class_init (PhocViewChildClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_view_child_get_property;
  object_class->set_property = phoc_view_child_set_property;
  object_class->constructed = phoc_view_child_constructed;
  object_class->finalize = phoc_view_child_finalize;

  props[PROP_VIEW] =
    g_param_spec_object ("view", "", "",
                         PHOC_TYPE_VIEW,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  props[PROP_WLR_SURFACE] =
    g_param_spec_pointer ("wlr-surface", "", "",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_view_child_init (PhocViewChild *self)
{
}
