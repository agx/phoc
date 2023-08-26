/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-view-deco"

#include "phoc-config.h"

#include "bling.h"
#include "view-deco.h"

#define PHOC_DECO_BORDER_WIDTH      4
#define PHOC_DECO_TITLEBAR_HEIGHT  12

/**
 * PhocViewDeco:
 *
 * The decoration for views using server side decorations
 */

enum {
  PROP_0,
  PROP_VIEW,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocViewDeco {
  GObject         parent;

  PhocView       *view;
  guint           border_width;
  guint           titlebar_height;
};

static void bling_interface_init (PhocBlingInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhocViewDeco, phoc_view_deco, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (PHOC_TYPE_BLING, bling_interface_init))


static PhocBox
phoc_view_deco_bling_get_box (PhocBling *bling)
{
  PhocViewDeco *self = PHOC_VIEW_DECO (bling);
  PhocBox box;

  phoc_view_get_box (self->view, &box);

  box.x -= self->border_width;
  box.y -= (self->border_width + self->titlebar_height);
  box.width += self->border_width * 2;
  box.height += (self->border_width * 2 + self->titlebar_height);

  return box;
}


static void
bling_interface_init (PhocBlingInterface *iface)
{
  iface->get_box = phoc_view_deco_bling_get_box;
}


static void
phoc_view_deco_set_view (PhocViewDeco *self, PhocView *view)
{
  g_assert (PHOC_IS_VIEW_DECO (self));

  if (self->view == view)
    return;

  if (self->view)
    g_object_remove_weak_pointer (G_OBJECT (self->view),
                                  (gpointer *)&self->view);

  self->view = view;

  if (self->view) {
    g_object_add_weak_pointer (G_OBJECT (self->view),
                               (gpointer *)&self->view);
  }
}


static void
phoc_view_deco_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  PhocViewDeco *self = PHOC_VIEW_DECO (object);

  switch (property_id) {
  case PROP_VIEW:
    phoc_view_deco_set_view (self, g_value_get_object (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_view_deco_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  PhocViewDeco *self = PHOC_VIEW_DECO (object);

  switch (property_id) {
  case PROP_VIEW:
    g_value_set_object (value, self->view);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}



static void
phoc_view_deco_dispose (GObject *object)
{
  PhocViewDeco *self = PHOC_VIEW_DECO (object);

  phoc_view_deco_set_view (self, NULL);

  G_OBJECT_CLASS (phoc_view_deco_parent_class)->dispose (object);
}


static void
phoc_view_deco_class_init (PhocViewDecoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_view_deco_get_property;
  object_class->set_property = phoc_view_deco_set_property;
  object_class->dispose = phoc_view_deco_dispose;

  props[PROP_VIEW] =
    g_param_spec_object ("view", "", "",
                         PHOC_TYPE_VIEW,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_view_deco_init (PhocViewDeco *self)
{
  self->border_width = PHOC_DECO_BORDER_WIDTH;
  self->titlebar_height = PHOC_DECO_TITLEBAR_HEIGHT;
}


PhocViewDeco *
phoc_view_deco_new (PhocView *view)
{
  return PHOC_VIEW_DECO (g_object_new (PHOC_TYPE_VIEW_DECO, "view", view, NULL));
}


PhocViewDecoPart
phoc_view_deco_get_part (PhocViewDeco *self, double sx, double sy)
{
  g_assert (PHOC_IS_VIEW_DECO (self));

  int sw = self->view->wlr_surface->current.width;
  int sh = self->view->wlr_surface->current.height;
  int bw = self->border_width;
  int titlebar_h = self->titlebar_height;

  if (sx > 0 && sx < sw && sy < 0 && sy > -titlebar_h)
    return PHOC_VIEW_DECO_PART_TITLEBAR;

  PhocViewDecoPart parts = 0;
  if (sy >= -(titlebar_h + bw) && sy <= sh + bw) {
    if (sx < 0 && sx > -bw)
      parts |= PHOC_VIEW_DECO_PART_LEFT_BORDER;
    else if (sx > sw && sx < sw + bw)
      parts |= PHOC_VIEW_DECO_PART_RIGHT_BORDER;
  }

  if (sx >= -bw && sx <= sw + bw) {
    if (sy > sh && sy <= sh + bw)
      parts |= PHOC_VIEW_DECO_PART_BOTTOM_BORDER;
    else if (sy >= -(titlebar_h + bw) && sy < 0)
      parts |= PHOC_VIEW_DECO_PART_TOP_BORDER;
  }

  // TODO corners

  return parts;
}
