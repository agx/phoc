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

G_END_DECLS
