/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 *         Sebastian Krzyszkowiak
 */

#include "desktop.h"
#include "server.h"
#include "touch-point.h"
#include "utils.h"

#define TOUCH_POINT_SIZE 20
#define TOUCH_POINT_BORDER 0.1

#define COLOR_TRANSPARENT_WHITE    ((struct wlr_render_color){0.5f, 0.5f, 0.5f, 0.5f})

/**
 * PhocTouchPoint:
 *
 * A touch point tracked compositor side.
 */
G_DEFINE_BOXED_TYPE (PhocTouchPoint, phoc_touch_point, phoc_touch_point_copy,
                     phoc_touch_point_destroy)

static void
color_hsv_to_rgb (struct wlr_render_color *color)
{
  float h = color->r, s = color->g, v = color->b;

  h = fmodf (h, 360);
  if (h < 0)
    h += 360;

  int d = h / 60;
  float e = h / 60 - d;
  float a = v * (1 - s);
  float b = v * (1 - e * s);
  float c = v * (1 - (1 - e) * s);

  switch (d) {
  default:
  case 0: color->r = v, color->g = c, color->b = a; return;
  case 1: color->r = b, color->g = v, color->b = a; return;
  case 2: color->r = a, color->g = v, color->b = c; return;
  case 3: color->r = a, color->g = b, color->b = v; return;
  case 4: color->r = c, color->g = a, color->b = v; return;
  case 5: color->r = v, color->g = a, color->b = b; return;
  }
}

/**
 * phoc_touch_point_get_box:
 * @self: The touch point
 * @output: The output the touch point is on
 * @width: The boxes width
 * @height: The boxes height
 *
 * Gets a box around the given touchpoint on output in output local coordinates.
 */
static struct wlr_box
phoc_touch_point_get_box (PhocTouchPoint *self, PhocOutput *output, int width, int height)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  double ox = self->lx, oy = self->ly;

  wlr_output_layout_output_coords (desktop->layout, output->wlr_output, &ox, &oy);
  return (struct wlr_box) {
           .x = ox - width / 2.0,
           .y = oy - height / 2.0,
           .width = width,
           .height = height
  };
}


void
phoc_touch_point_damage (PhocTouchPoint *self)
{
  PhocServer *server = phoc_server_get_default ();
  PhocDesktop *desktop = phoc_server_get_desktop (server);
  PhocOutput *output;

  if (!G_UNLIKELY (phoc_server_check_debug_flags (server, PHOC_SERVER_DEBUG_FLAG_TOUCH_POINTS)))
    return;

  wl_list_for_each (output, &desktop->outputs, link) {
    if (wlr_output_layout_contains_point (desktop->layout, output->wlr_output, self->lx,
                                          self->ly)) {
      int size = TOUCH_POINT_SIZE;
      struct wlr_box box;

      box = phoc_touch_point_get_box (self, output, size, size);
      phoc_utils_scale_box (&box, output->wlr_output->scale);

      if (wlr_damage_ring_add_box (&output->damage_ring, &box))
        wlr_output_schedule_frame (output->wlr_output);
    }
  }
}


PhocTouchPoint *
phoc_touch_point_new (int touch_id, double lx, double ly)
{
  PhocTouchPoint *self = g_new0 (PhocTouchPoint, 1);

  self->touch_id = touch_id;
  self->lx = lx;
  self->ly = ly;

  phoc_touch_point_damage (self);

  return self;
}


void
phoc_touch_point_destroy (PhocTouchPoint *self)
{
  phoc_touch_point_damage (self);

  g_free (self);
}


PhocTouchPoint *
phoc_touch_point_copy (PhocTouchPoint *self)
{
  return phoc_touch_point_new (self->touch_id, self->lx, self->ly);
}


void
phoc_touch_point_update (PhocTouchPoint *self, double lx, double ly)
{
  g_assert (self);

  phoc_touch_point_damage (self);

  self->lx = lx;
  self->ly = ly;

  phoc_touch_point_damage (self);
}


void
phoc_touch_point_render (PhocTouchPoint *self, PhocRenderContext *ctx)
{
  PhocServer *server = phoc_server_get_default ();
  PhocDesktop *desktop = phoc_server_get_desktop (server);
  struct wlr_output *wlr_output = ctx->output->wlr_output;
  struct wlr_render_color color = {self->touch_id * 100 + 240, 1.0, 1.0, 0.75};
  struct wlr_box point_box;
  int size;

  g_assert (phoc_server_check_debug_flags (server, PHOC_SERVER_DEBUG_FLAG_TOUCH_POINTS));

  if (!wlr_output_layout_contains_point (desktop->layout, wlr_output, self->lx, self->ly))
    return;

  color_hsv_to_rgb (&color);

  point_box = phoc_touch_point_get_box (self, ctx->output, TOUCH_POINT_SIZE, TOUCH_POINT_SIZE);
  phoc_utils_scale_box (&point_box, ctx->output->wlr_output->scale);
  phoc_output_transform_box (ctx->output, &point_box);
  wlr_render_pass_add_rect (ctx->render_pass, &(struct wlr_render_rect_options){
    .box = point_box,
    .color = color,
  });

  size = TOUCH_POINT_SIZE * (1.0 - TOUCH_POINT_BORDER);
  point_box = phoc_touch_point_get_box (self, ctx->output, size, size);
  phoc_utils_scale_box (&point_box, ctx->output->wlr_output->scale);
  phoc_output_transform_box (ctx->output, &point_box);
  wlr_render_pass_add_rect (ctx->render_pass, &(struct wlr_render_rect_options){
    .box = point_box,
    .color = COLOR_TRANSPARENT_WHITE,
  });

  point_box = phoc_touch_point_get_box (self, ctx->output, 8, 2);
  phoc_utils_scale_box (&point_box, ctx->output->wlr_output->scale);
  phoc_output_transform_box (ctx->output, &point_box);
  wlr_render_pass_add_rect (ctx->render_pass, &(struct wlr_render_rect_options){
    .box = point_box,
    .color = color,
  });

  point_box = phoc_touch_point_get_box (self, ctx->output, 2, 8);
  phoc_utils_scale_box (&point_box, ctx->output->wlr_output->scale);
  phoc_output_transform_box (ctx->output, &point_box);
  wlr_render_pass_add_rect (ctx->render_pass, &(struct wlr_render_rect_options){
    .box = point_box,
    .color = color,
  });
}
