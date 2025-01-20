/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

#define PHOC_TYPE_TOUCH_POINT (phoc_touch_point_get_type ())

typedef struct PhocTouchPoint {
  int    touch_id;

  double lx;
  double ly;
} PhocTouchPoint;

GType           phoc_touch_point_get_type (void);

PhocTouchPoint *phoc_touch_point_new (int touch_id, double lx, double ly);
PhocTouchPoint *phoc_touch_point_copy (PhocTouchPoint *self);
void            phoc_touch_point_destroy (PhocTouchPoint *self);

void            phoc_touch_point_update (PhocTouchPoint *self, double lx, double ly);

void            phoc_touch_point_render (PhocTouchPoint    *self,
                                         PhocRenderContext *ctx);
void            phoc_touch_point_damage (PhocTouchPoint *self);

G_END_DECLS
