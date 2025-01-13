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

/**
 * PhocTouchPoint:
 *
 * A touch point tracked compositor side.
 */


static void
damage_touch_point (PhocTouchPoint *self)
{
  PhocDesktop *desktop = phoc_server_get_desktop (phoc_server_get_default ());
  PhocOutput *output;

  wl_list_for_each (output, &desktop->outputs, link) {
    if (wlr_output_layout_contains_point (desktop->layout, output->wlr_output, self->lx, self->ly)) {
      double ox = self->lx, oy = self->ly;
      wlr_output_layout_output_coords (desktop->layout, output->wlr_output, &ox, &oy);
      struct wlr_box box = {
        .x = ox,
        .y = oy,
        .width = 1,
        .height = 1
      };
      wlr_damage_ring_add_box (&output->damage_ring, &box);
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
    damage_touch_point (self);

  return self;
}


void
phoc_touch_point_destroy (PhocTouchPoint *self)
{
  g_free (self);
}
