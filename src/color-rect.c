/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-color-box"

#include "phoc-config.h"
#include "color-rect.h"
#include "server.h"
#include "desktop.h"
#include "output.h"
#include "utils.h"

/**
 * PhocColorRect:
 *
 * A colored rectangle to be drawn by the compositor.
 *
 * When created the rectangle is initially unmapped. For it to be drawn it needs
 * to be mapped and attached to the render tree by e.g. adding it as a [type@Bling]
 * to a [type@View].
 */

enum {
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_BOX,
  PROP_COLOR,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

struct _PhocColorRect {
  GObject        parent;

  gboolean       mapped;
  PhocBox        box;
  PhocColor      color;
};

static void bling_interface_init (PhocBlingInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhocColorRect, phoc_color_rect, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (PHOC_TYPE_BLING, bling_interface_init))


static void
phoc_color_rect_damage_box (PhocColorRect *self)
{
  PhocDesktop *desktop = phoc_server_get_default ()->desktop;
  PhocOutput *output;

  if (!self->mapped)
    return;

  wl_list_for_each (output, &desktop->outputs, link) {
    struct wlr_box damage_box = self->box;
    bool intersects = wlr_output_layout_intersects (desktop->layout, output->wlr_output, &self->box);
    if (!intersects)
      continue;

    damage_box.x -= output->lx;
    damage_box.y -= output->ly;
    phoc_utils_scale_box (&damage_box, output->wlr_output->scale);

    if (wlr_damage_ring_add_box (&output->damage_ring, &damage_box))
      wlr_output_schedule_frame (output->wlr_output);
  }
}


static void
phoc_color_rect_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  PhocColorRect *self = PHOC_COLOR_RECT (object);

  switch (property_id) {
  case PROP_X:
    /* Damage the old box's area */
    phoc_color_rect_damage_box (self);
    self->box.x = g_value_get_int (value);
    /* Damage the new box's area */
    phoc_color_rect_damage_box (self);
    break;
  case PROP_Y:
    phoc_color_rect_damage_box (self);
    self->box.y = g_value_get_int (value);
    phoc_color_rect_damage_box (self);
    break;
  case PROP_WIDTH:
    phoc_color_rect_damage_box (self);
    self->box.width = g_value_get_uint (value);
    phoc_color_rect_damage_box (self);
    break;
  case PROP_HEIGHT:
    phoc_color_rect_damage_box (self);
    self->box.height = g_value_get_uint (value);
    phoc_color_rect_damage_box (self);
    break;
  case PROP_BOX:
    phoc_color_rect_damage_box (self);
    self->box = *(PhocBox*)g_value_get_boxed (value);
    phoc_color_rect_damage_box (self);
    break;
  case PROP_COLOR:
    self->color = *(PhocColor*)g_value_get_boxed (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_color_rect_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  PhocColorRect *self = PHOC_COLOR_RECT (object);

  switch (property_id) {
  case PROP_X:
    g_value_set_int (value, self->box.x);
    break;
  case PROP_Y:
    g_value_set_int (value, self->box.y);
    break;
  case PROP_WIDTH:
    g_value_set_uint (value, self->box.width);
    break;
  case PROP_HEIGHT:
    g_value_set_uint (value, self->box.height);
    break;
  case PROP_BOX:
    g_value_set_boxed (value, &self->box);
    break;
  case PROP_COLOR:
    g_value_set_boxed (value, &self->color);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_color_rect_dispose (GObject *object)
{
  PhocColorRect *self = PHOC_COLOR_RECT(object);

  phoc_bling_unmap (PHOC_BLING (self));

  G_OBJECT_CLASS (phoc_color_rect_parent_class)->dispose (object);
}

static void
bling_render (PhocBling *bling, PhocOutput *output)
{
  PhocColorRect *self = PHOC_COLOR_RECT (bling);

  struct wlr_box box = self->box;
  box.x -= output->lx;
  box.y -= output->ly;
  phoc_utils_scale_box (&box, output->wlr_output->scale);

  wlr_render_rect (output->wlr_output->renderer, &box,
                   (float []){self->color.red,
                     self->color.green,
                     self->color.blue,
                     self->color.alpha},
                   output->wlr_output->transform_matrix);
}


static PhocBox
bling_get_box (PhocBling *bling)
{
  PhocColorRect *self = PHOC_COLOR_RECT (bling);

  return phoc_color_rect_get_box (self);
}


static void
bling_map (PhocBling *bling)
{
  PhocColorRect *self = PHOC_COLOR_RECT (bling);

  self->mapped = TRUE;
  phoc_color_rect_damage_box (self);
}


static void
bling_unmap (PhocBling *bling)
{
  PhocColorRect *self = PHOC_COLOR_RECT (bling);

  phoc_color_rect_damage_box (self);
  self->mapped = FALSE;
}


static gboolean
bling_is_mapped (PhocBling *bling)
{
  PhocColorRect *self = PHOC_COLOR_RECT (bling);

  return self->mapped;
}


static void
bling_interface_init (PhocBlingInterface *iface)
{
  iface->get_box = bling_get_box;
  iface->render = bling_render;
  iface->map = bling_map;
  iface->unmap = bling_unmap;
  iface->is_mapped = bling_is_mapped;
}



static void
phoc_color_rect_class_init (PhocColorRectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_color_rect_get_property;
  object_class->set_property = phoc_color_rect_set_property;
  object_class->dispose = phoc_color_rect_dispose;

  props[PROP_X] =
    g_param_spec_int ("x", "", "",
                      -G_MAXINT, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_Y] =
    g_param_spec_int ("y", "", "",
                      -G_MAXINT, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_WIDTH] =
    g_param_spec_uint ("width", "", "",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  props[PROP_HEIGHT] =
    g_param_spec_uint ("height", "", "",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhocColorRect:box:
   *
   * The rectangle's box in layout coordinates
   */
  props[PROP_BOX] =
    g_param_spec_boxed ("box", "", "",
                        PHOC_TYPE_BOX,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  /**
   * PhocColorRect:color:
   *
   * The rectangle's color.
   */
  props[PROP_COLOR] =
    g_param_spec_boxed ("color", "", "",
                        PHOC_TYPE_COLOR,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_color_rect_init (PhocColorRect *self)
{
}


PhocColorRect *
phoc_color_rect_new (PhocBox *box, PhocColor *color)
{
  return PHOC_COLOR_RECT (g_object_new (PHOC_TYPE_COLOR_RECT,
                                        "box", box,
                                        "color", color,
                                        NULL));
}


/**
 * phoc_color_rect_get_box:
 * @self: The color rectangle
 *
 * Get the rectangles current coordinates and size as box.
 *
 * Returns: The current rectangle's position and size
 */
PhocBox
phoc_color_rect_get_box (PhocColorRect *self)
{
  g_assert (PHOC_IS_COLOR_RECT (self));

  return self->box;
}

/**
 * phoc_color_rect_get_color:
 * @self: The color rectangle
 *
 * Get the rectangles color
 *
 * Returns: the color
 */
PhocColor
phoc_color_rect_get_color (PhocColorRect *self)
{
  g_assert (PHOC_IS_COLOR_RECT (self));

  return self->color;
}
