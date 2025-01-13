/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Author: Guido Günther <agx@sigxcpu.org>
 */

#include "touch-point.h"

/**
 * PhocTouchPoint:
 *
 * A touch point tracked compositor side.
 */

PhocTouchPoint *
phoc_touch_point_new (int touch_id, double lx, double ly)
{
  PhocTouchPoint *self = g_new0 (PhocTouchPoint, 1);

  self->touch_id = touch_id;
  self->lx = lx;
  self->ly = ly;

  return self;
}


void
phoc_touch_point_destroy (PhocTouchPoint *self)
{
  g_free (self);
}
