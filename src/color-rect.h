/*
 * Copyright (C) 2023 Guido Günther
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "phoc-types.h"
#include "bling.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_COLOR_RECT (phoc_color_rect_get_type ())

G_DECLARE_FINAL_TYPE (PhocColorRect, phoc_color_rect, PHOC, COLOR_RECT, GObject)

PhocColorRect      *phoc_color_rect_new                         (PhocBox       *box,
                                                                 PhocColor     *color);
PhocBox             phoc_color_rect_get_box                     (PhocColorRect *self) G_GNUC_WARN_UNUSED_RESULT;
PhocColor           phoc_color_rect_get_color                   (PhocColorRect *self) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS
