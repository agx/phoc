/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef struct PhocTouchPoint {
  int    touch_id;

  double lx;
  double ly;
} PhocTouchPoint;


PhocTouchPoint *phoc_touch_point_new (int touch_id, double lx, double ly);
void            phoc_touch_point_destroy (PhocTouchPoint *self);

struct wlr_box  phoc_touch_point_get_box (PhocTouchPoint *self,
                                          PhocOutput     *output,
                                          int             width,
                                          int             height);

G_END_DECLS
