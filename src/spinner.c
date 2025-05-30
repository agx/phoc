/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * Portions of this code came from AdwSpinnerPaintable, which retains the following copyright:
 * Copyright (C) 2024 Alice Mikhaylenko <alicem@gnome.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "phoc-spinner"

#include "phoc-types.h"

#include "bling.h"
#include "cairo-texture.h"
#include "server.h"
#include "spinner.h"
#include "timed-animation.h"

#include "render-private.h"

#include <cairo.h>

enum {
  PROP_0,
  PROP_LX,
  PROP_LY,
  PROP_SIZE,
  PROP_ANIMATABLE,
  PROP_ANGLE,
  PROP_LAST_PROP
};

static GParamSpec *props[PROP_LAST_PROP];

#define MIN_RADIUS 8
#define MAX_RADIUS 64
#define SMALL_WIDTH 2.5
#define LARGE_WIDTH 14
#define SPIN_DURATION_MS 1200
#define START_ANGLE (G_PI * 0.35)
#define CIRCLE_OPACITY 0.15
#define MIN_ARC_LENGTH (G_PI * 0.015)
#define MAX_ARC_LENGTH (G_PI * 0.9)
#define IDLE_DISTANCE (G_PI * 0.9)
#define OVERLAP_DISTANCE (G_PI * 0.7)
#define EXTEND_DISTANCE (G_PI * 1.1)
#define CONTRACT_DISTANCE (G_PI * 1.35)
/* How many full cycles it takes for the spinner to loop. Should be:
 * (IDLE_DISTANCE + EXTEND_DISTANCE + CONTRACT_DISTANCE - OVERLAP_DISTANCE) * k,
 * where k is an integer */
#define N_CYCLES 53

/**
 * PhocSpinner:
 *
 * An animated spinner, used to represent indeterminate progress. It is rendered as a [type@Bling].
 */
struct _PhocSpinner {
  GObject             parent;

  int                 lx;
  int                 ly;
  int                 size;
  PhocAnimatable     *animatable;
  PhocTimedAnimation *animation;
  PhocPropertyEaser  *easer;
  float               angle;
  PhocCairoTexture   *texture;
  gboolean            redraw_spinner; /* set whenever angle changes */
};

static void bling_interface_init (PhocBlingInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhocSpinner, phoc_spinner, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (PHOC_TYPE_BLING, bling_interface_init))


static void
on_animation_done (PhocSpinner *self)
{
  /* animation cycles indefinitely */
  phoc_timed_animation_play (self->animation);
}


static PhocBox
bling_get_box (PhocBling *bling)
{
  PhocSpinner *self = PHOC_SPINNER (bling);

  return (PhocBox) {
           .x = self->lx,
           .y = self->ly,
           .width = self->size,
           .height = self->size,
  };
}


static double
inverse_lerp (double a, double b, double t)
{
  return (t - a) / (b - a);
}


static double
normalize_angle (double angle)
{
  while (angle < 0)
    angle += G_PI * 2;

  while (angle > G_PI * 2)
    angle -= G_PI * 2;

  return angle;
}

static double
get_arc_start (double angle)
{
  float l = IDLE_DISTANCE + EXTEND_DISTANCE + CONTRACT_DISTANCE - OVERLAP_DISTANCE;
  double t;

  angle = fmod (angle, l);

  if (angle > EXTEND_DISTANCE) {
    t = 1;
  } else {
    t = angle / EXTEND_DISTANCE;
    t = phoc_easing_ease (PHOC_EASING_EASE_IN_OUT_SINE, t);
  }

  return phoc_lerp (MIN_ARC_LENGTH, MAX_ARC_LENGTH, t) - angle * MAX_ARC_LENGTH / l;
}

static double
get_arc_end (double angle)
{
  float l = IDLE_DISTANCE + EXTEND_DISTANCE + CONTRACT_DISTANCE - OVERLAP_DISTANCE;
  double t;

  angle = fmod (angle, l);

  if (angle < EXTEND_DISTANCE - OVERLAP_DISTANCE) {
    t = 0;
  } else if (angle > l - IDLE_DISTANCE) {
    t = 1;
  } else {
    t = (angle - EXTEND_DISTANCE + OVERLAP_DISTANCE) / CONTRACT_DISTANCE;
    t = phoc_easing_ease (PHOC_EASING_EASE_IN_OUT_SINE, t);
  }

  return phoc_lerp (0, MAX_ARC_LENGTH - MIN_ARC_LENGTH, t) - angle * MAX_ARC_LENGTH / l;
}


static void
draw_spinner (cairo_t *cr, double base_angle, double size)
{
  double radius, line_width;
  double start_angle, end_angle;

  cairo_save (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_restore (cr);

  radius = (float)size / 2;
  line_width = phoc_lerp (SMALL_WIDTH, LARGE_WIDTH,
                          inverse_lerp (MIN_RADIUS, MAX_RADIUS, radius));

  if (radius < line_width / 2)
    return;

  cairo_save (cr);
  cairo_set_line_width (cr, line_width);
  cairo_translate (cr, size / 2, size / 2);

  /* circle */
  cairo_set_source_rgba (cr, CIRCLE_OPACITY, CIRCLE_OPACITY, CIRCLE_OPACITY, 1.0);
  cairo_arc (cr, 0, 0, radius - line_width / 2, 0, 2.0 * M_PI);
  cairo_stroke (cr);

  /* animated arc */
  start_angle = base_angle + get_arc_start (base_angle) + START_ANGLE;
  end_angle = base_angle + get_arc_end (base_angle) + START_ANGLE;

  start_angle = normalize_angle (start_angle);
  end_angle = normalize_angle (end_angle);

  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, .55);
  cairo_arc_negative (cr, 0, 0, radius - line_width / 2, start_angle, end_angle);
  cairo_stroke (cr);

  cairo_restore (cr);
}


static void
bling_render (PhocBling *bling, PhocRenderContext *ctx)
{
  PhocSpinner *self = PHOC_SPINNER (bling);
  struct wlr_render_texture_options options;
  struct wlr_box box = bling_get_box (bling);
  pixman_region32_t damage;
  struct wlr_texture *texture = phoc_cairo_texture_get_texture (self->texture);

  if (!texture)
    return;

  box.x -= ctx->output->lx;
  box.y -= ctx->output->ly;
  phoc_utils_scale_box (&box, ctx->output->wlr_output->scale);
  phoc_output_transform_box (ctx->output, &box);

  if (!phoc_utils_is_damaged (&box, ctx->damage, NULL, &damage)) {
    pixman_region32_fini (&damage);
    return;
  }

  /* only redraw spinner if needed (angle property was changed since last render) */
  if (self->redraw_spinner) {
    draw_spinner (phoc_cairo_texture_get_context (self->texture), self->angle, box.width);
    phoc_cairo_texture_update (self->texture);
    self->redraw_spinner = false;
  }

  options = (struct wlr_render_texture_options) {
    .texture = texture,
    .dst_box = box,
    .clip    = &damage,
  };

  wlr_render_pass_add_texture (ctx->render_pass, &options);
}


static void
bling_map (PhocBling *bling)
{
  PhocSpinner *self = PHOC_SPINNER (bling);
  PhocServer *server = phoc_server_get_default ();
  PhocDesktop *desktop = phoc_server_get_desktop (server);
  PhocOutput *output = phoc_desktop_layout_get_output (desktop, self->lx, self->ly);
  int size = self->size;

  if (output)
    size = size * phoc_output_get_scale (output);

  cairo_t *cr;

  if (self->texture)
    return;

  self->texture = phoc_cairo_texture_new (size, size);
  cr = phoc_cairo_texture_get_context (self->texture);

  if (!cr) {
    g_warning ("No Cairo context, cannot render spinner\n");
    return;
  }

  cairo_set_antialias (cr, CAIRO_ANTIALIAS_FAST);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

  phoc_bling_damage_box (PHOC_BLING (self));
  phoc_timed_animation_play (self->animation);
}


static void
bling_unmap (PhocBling *bling)
{
  PhocSpinner *self = PHOC_SPINNER (bling);

  if (!self->texture)
    return;

  g_clear_object (&self->texture);

  phoc_bling_damage_box (PHOC_BLING (self));
  phoc_timed_animation_reset (self->animation);
}


static gboolean
bling_is_mapped (PhocBling *bling)
{
  PhocSpinner *self = PHOC_SPINNER (bling);

  return self->texture != NULL;
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
phoc_spinner_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PhocSpinner *self = PHOC_SPINNER (object);

  switch (property_id) {
  case PROP_LX:
    phoc_bling_damage_box (PHOC_BLING (self));
    self->lx = g_value_get_int (value);
    phoc_bling_damage_box (PHOC_BLING (self));
    break;
  case PROP_LY:
    phoc_bling_damage_box (PHOC_BLING (self));
    self->ly = g_value_get_int (value);
    phoc_bling_damage_box (PHOC_BLING (self));
    break;
  case PROP_SIZE:
    self->size = g_value_get_int (value);
    break;
  case PROP_ANIMATABLE:
    self->animatable = g_value_get_object (value);
    break;
  case PROP_ANGLE:
    self->angle = g_value_get_float (value);
    phoc_bling_damage_box (PHOC_BLING (self));
    self->redraw_spinner = true;
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_spinner_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PhocSpinner *self = PHOC_SPINNER (object);

  switch (property_id) {
  case PROP_LX:
    g_value_set_int (value, self->lx);
    break;
  case PROP_LY:
    g_value_set_int (value, self->ly);
    break;
  case PROP_SIZE:
    g_value_set_int (value, self->size);
    break;
  case PROP_ANIMATABLE:
    g_value_set_object (value, self->animatable);
    break;
  case PROP_ANGLE:
    g_value_set_float (value, self->angle);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_spinner_finalize (GObject *object)
{
  PhocSpinner *self = PHOC_SPINNER (object);

  phoc_bling_unmap (PHOC_BLING (self));
  g_clear_object (&self->easer);
  g_clear_object (&self->animation);

  G_OBJECT_CLASS (phoc_spinner_parent_class)->finalize (object);
}


static void
phoc_spinner_constructed (GObject *object)
{
  PhocSpinner *self = PHOC_SPINNER (object);

  G_OBJECT_CLASS (phoc_spinner_parent_class)->constructed (object);

  self->animation = g_object_new (PHOC_TYPE_TIMED_ANIMATION,
                                  "animatable", g_steal_pointer (&self->animatable),
                                  "duration", SPIN_DURATION_MS * N_CYCLES,
                                  "property-easer", self->easer,
                                  NULL);
  g_signal_connect_swapped (self->animation, "done",
                            G_CALLBACK (on_animation_done),
                            self);
}


static void
phoc_spinner_class_init (PhocSpinnerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = phoc_spinner_constructed;
  object_class->get_property = phoc_spinner_get_property;
  object_class->set_property = phoc_spinner_set_property;
  object_class->finalize = phoc_spinner_finalize;

  /**
   * PhocSpinner:animatable:
   *
   * A [type@Animatable] implementation that can drive this spinner's animation
   */
  props[PROP_ANIMATABLE] =
    g_param_spec_object ("animatable", "", "",
                         PHOC_TYPE_ANIMATABLE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  /**
   * PhocSpinner:lx:
   *
   * The x coord to render spinner at
   */
  props[PROP_LX] =
    g_param_spec_int ("lx", "", "",
                      0, INT32_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PhocSpinner:ly:
   *
   * The y coord to render spinner at
   */
  props[PROP_LY] =
    g_param_spec_int ("ly", "", "",
                      0, INT32_MAX, 0,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PhocSpinner:angle:
   *
   * The current angle of the spinner.
   */
  props[PROP_ANGLE] =
    g_param_spec_float ("angle", "", "",
                        0,
                        N_CYCLES * G_PI * 2,
                        0.0f,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * PhocSpinner:size:
   *
   * The width and height of the spinner.
   */
  props[PROP_SIZE] =
    g_param_spec_int ("size", "", "",
                      16,
                      64,
                      16,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_spinner_init (PhocSpinner *self)
{
  self->easer = g_object_new (PHOC_TYPE_PROPERTY_EASER,
                              "target", self,
                              "easing", PHOC_EASING_NONE,
                              NULL);
  phoc_property_easer_set_props (self->easer,
                                 "angle", 0.0, N_CYCLES * G_PI * 2,
                                 NULL);
}


PhocSpinner*
phoc_spinner_new (PhocAnimatable *animatable, int lx, int ly, int size)
{
  return g_object_new (PHOC_TYPE_SPINNER,
                       "animatable", animatable,
                       "lx", lx,
                       "ly", ly,
                       "size", size,
                       NULL);
}
