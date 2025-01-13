/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "desktop.h"
#include "server.h"
#include "touch-point.h"
#include "utils.h"

#define TOUCH_POINT_SIZE 20
#define TOUCH_POINT_BORDER 0.1

/**
 * PhocTouchPoint:
 *
 * A touch point tracked compositor side.
 */

/**
 * phoc_touch_point_get_box:
 * @self: The touch point
 * @output: The output the touch point is on
 * @width: The boxes width
 * @height: The boxes height
 *
 * Gets a box around the given touchpoint on output in output local coordinates.
 */
struct wlr_box
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
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocOutput *output;

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
  PhocServer *server = phoc_server_get_default ();
  PhocTouchPoint *self = g_new0 (PhocTouchPoint, 1);

  self->touch_id = touch_id;
  self->lx = lx;
  self->ly = ly;

  if (G_UNLIKELY (phoc_server_check_debug_flags (server, PHOC_SERVER_DEBUG_FLAG_TOUCH_POINTS)))
    phoc_touch_point_damage (self);

  return self;
}


void
phoc_touch_point_destroy (PhocTouchPoint *self)
{
  g_free (self);
}
