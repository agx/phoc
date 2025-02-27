/*
 * Copyright (C) 2022 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-output-shield"

#include "phoc-config.h"

#include "color-rect.h"
#include "output-shield.h"
#include "phoc-animation.h"
#include "server.h"
#include "spinner.h"

#include "render-private.h"

#define PHOC_ANIM_DURATION_SHIELD_MS 250 /* ms */
#define SPINNER_SIZE 32

enum {
  PROP_0,
  PROP_OUTPUT,
  PROP_EASING,
  PROP_LAST_PROP
};
static GParamSpec *props[PROP_LAST_PROP];

/**
 * PhocOutputShield:
 *
 * A shield that covers a whole `PhocOutput`. It can be raised (to cover
 * the whole screen) and lowered to show the screens content.
 */
struct _PhocOutputShield {
  GObject             parent;

  PhocColorRect      *color_rect;
  PhocSpinner        *spinner;
  PhocOutput         *output;
  PhocTimedAnimation *animation;
  PhocPropertyEaser  *easer;

  gulong              render_end_id;
};

static void phoc_output_shield_animatable_interface_init (PhocAnimatableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (PhocOutputShield, phoc_output_shield, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (PHOC_TYPE_ANIMATABLE,
                                                phoc_output_shield_animatable_interface_init))

static guint
phoc_output_shield_add_frame_callback (PhocAnimatable   *iface,
                                       PhocFrameCallback callback,
                                       gpointer          user_data,
                                       GDestroyNotify    notify)
{
  PhocOutputShield *self = PHOC_OUTPUT_SHIELD (iface);

  return phoc_output_add_frame_callback (self->output, iface, callback, user_data, notify);
}


static void
phoc_output_shield_remove_frame_callback (PhocAnimatable *iface, guint id)
{
  PhocOutputShield *self = PHOC_OUTPUT_SHIELD (iface);

  phoc_output_remove_frame_callback (self->output, id);
}


static void
set_output (PhocOutputShield *self, PhocOutput *output)
{
  g_assert (output == NULL || PHOC_IS_OUTPUT (output));

  if (self->output == output)
    return;

  g_set_weak_pointer (&self->output, output);
}


static void
phoc_output_shield_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  PhocOutputShield *self = PHOC_OUTPUT_SHIELD (object);

  switch (property_id) {
  case PROP_OUTPUT:
    set_output (self, g_value_get_object (value));
    break;
  case PROP_EASING:
    phoc_output_shield_set_easing (self, g_value_get_enum (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
phoc_output_shield_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  PhocOutputShield *self = PHOC_OUTPUT_SHIELD (object);

  switch (property_id) {
  case PROP_OUTPUT:
    g_value_set_object (value, self->output);
    break;
  case PROP_EASING:
    g_value_set_enum (value, phoc_property_easer_get_easing (self->easer));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
stop_render (PhocOutputShield *self)
{
  PhocRenderer *renderer = phoc_server_get_renderer (phoc_server_get_default ());

  phoc_bling_unmap (PHOC_BLING (self->color_rect));
  if (self->spinner)
    phoc_bling_unmap (PHOC_BLING (self->spinner));

  g_clear_signal_handler (&self->render_end_id, renderer);
}



static void
on_render (PhocOutputShield *self, PhocRenderContext *ctx)
{
  if (self->output == NULL || self->output != ctx->output)
    return;

  phoc_bling_render (PHOC_BLING (self->color_rect), ctx);
  if (self->spinner)
    phoc_bling_render (PHOC_BLING (self->spinner), ctx);
}


static void
start_render (PhocOutputShield *self, gboolean show_spinner)
{
  PhocServer *server = phoc_server_get_default ();
  PhocDesktop *desktop = phoc_server_get_desktop (server);
  PhocRenderer *renderer = phoc_server_get_renderer (server);
  PhocBox output_box;
  struct wlr_output *output = self->output->wlr_output;

  if (self->render_end_id)
    return;

  wlr_output_layout_get_box (desktop->layout, output, &output_box);

  phoc_color_rect_set_box (self->color_rect, &output_box);
  phoc_bling_map (PHOC_BLING (self->color_rect));

  if (show_spinner && !self->spinner) {
    int lx, ly;

    lx = output_box.x + output_box.width * 0.5 - SPINNER_SIZE * 0.5;
    ly = output_box.y + output_box.height * 0.5 - SPINNER_SIZE * 0.5;

    self->spinner = phoc_spinner_new (PHOC_ANIMATABLE (self), lx, ly, SPINNER_SIZE);
  }

  if (show_spinner)
    phoc_bling_map (PHOC_BLING (self->spinner));

  self->render_end_id = g_signal_connect_swapped (renderer,
                                                  "render-end",
                                                  G_CALLBACK (on_render),
                                                  self);
}


static void
on_animation_done (PhocOutputShield *self)
{
  /* We can unhook from the render loop once the shield is lowered completely */
  stop_render (self);
}


static void
phoc_output_shield_finalize (GObject *object)
{
  PhocOutputShield *self = PHOC_OUTPUT_SHIELD (object);

  set_output (self, NULL);
  stop_render (self);

  g_clear_object (&self->color_rect);
  g_clear_object (&self->easer);
  g_clear_object (&self->animation);
  g_clear_object (&self->spinner);

  G_OBJECT_CLASS (phoc_output_shield_parent_class)->finalize (object);
}


static void
phoc_output_shield_animatable_interface_init (PhocAnimatableInterface *iface)
{
  iface->add_frame_callback = phoc_output_shield_add_frame_callback;
  iface->remove_frame_callback = phoc_output_shield_remove_frame_callback;
}


static void
phoc_output_shield_class_init (PhocOutputShieldClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = phoc_output_shield_get_property;
  object_class->set_property = phoc_output_shield_set_property;
  object_class->finalize = phoc_output_shield_finalize;

  /**
   * PhocOutputShield:output:
   *
   * The output covered by this shield.
   */
  props[PROP_OUTPUT] =
    g_param_spec_object ("output", "", "",
                         PHOC_TYPE_OUTPUT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  /**
   * PhocOutputShield:easing:
   *
   * The easing function to use
   */
  props[PROP_EASING] =
    g_param_spec_enum ("easing", "", "",
                       PHOC_TYPE_EASING,
                       PHOC_EASING_EASE_IN_CUBIC,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);
}


static void
phoc_output_shield_init (PhocOutputShield *self)
{
  g_autoptr (PhocTimedAnimation) fade_anim = NULL;

  self->color_rect = phoc_color_rect_new (&(PhocBox){}, &(PhocColor){0.0f, 0.0f, 0.0f, 1.0f});

  self->easer = g_object_new (PHOC_TYPE_PROPERTY_EASER,
                              "target", self->color_rect,
                              "easing", PHOC_EASING_EASE_IN_CUBIC,
                              NULL);
  phoc_property_easer_set_props (self->easer,
                                 "alpha", 1.0, 0.0,
                                 NULL);

  fade_anim = g_object_new (PHOC_TYPE_TIMED_ANIMATION,
                            "animatable", self,
                            "duration", PHOC_ANIM_DURATION_SHIELD_MS,
                            "property-easer", self->easer,
                            NULL);
  g_set_object (&self->animation, fade_anim);

  g_signal_connect_swapped (self->animation, "done",
                            G_CALLBACK (on_animation_done),
                            self);
}


PhocOutputShield *
phoc_output_shield_new (PhocOutput *output)
{
  return g_object_new (PHOC_TYPE_OUTPUT_SHIELD,
                       "output", output,
                       NULL);
}


/**
 * phoc_output_shield_raise:
 * @self: The shield
 * @show_spinner: Whether to show a spinner on the raised shield
 *
 * Draw the shield to cover the whole output. A spinner can optionally be displayed on the shield.
 */
void
phoc_output_shield_raise (PhocOutputShield *self, gboolean show_spinner)
{
  g_return_if_fail (PHOC_IS_OUTPUT_SHIELD (self));

  phoc_timed_animation_skip (self->animation);

  phoc_color_rect_set_alpha (self->color_rect, 1.0f);

  start_render (self, show_spinner);
}

/**
 * phoc_output_shield_lower
 * @self: The shield
 *
 * Lower the shield exposing the output's content.
 */
void
phoc_output_shield_lower (PhocOutputShield *self)
{
  g_return_if_fail (PHOC_IS_OUTPUT_SHIELD (self));

  start_render (self, false);
  phoc_timed_animation_play (self->animation);
  g_clear_object (&self->spinner);
}


void
phoc_output_shield_set_easing (PhocOutputShield *self, PhocEasing easing)
{
  g_assert (PHOC_IS_OUTPUT_SHIELD (self));

  phoc_property_easer_set_easing (self->easer, easing);
}


void
phoc_output_shield_set_duration (PhocOutputShield *self, guint duration)
{
  g_assert (PHOC_IS_OUTPUT_SHIELD (self));

  if (duration == 0)
    duration = PHOC_ANIM_DURATION_SHIELD_MS;

  phoc_timed_animation_set_duration (self->animation, duration);
}

/**
 * phoc_output_shield_is_raised:
 * @self: The shield
 *
 * Check whether the shield is currently fully up (raised). We return  `FALSE`
 * when the shield is either down or already fading out.
 */
gboolean
phoc_output_shield_is_raised (PhocOutputShield *self)
{
  g_assert (PHOC_IS_OUTPUT_SHIELD (self));

  return G_APPROX_VALUE (phoc_color_rect_get_alpha (self->color_rect), 1.0, FLT_EPSILON);
}
