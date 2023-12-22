/*
 * Copyright (C) 2020,2021 Purism SPC
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Authors: Arnaud Ferraris <arnaud.ferraris@collabora.com>
 *          Clayton Craft <clayton@craftyguy.net>
 *          Guido Günther <agx@sigxcpu.org>
 */

#define G_LOG_DOMAIN "phoc-utils"

#include <inttypes.h>
#include <math.h>
#include <wlr/util/box.h>
#include "utils.h"


void
phoc_utils_fix_transform (enum wl_output_transform *transform)
{
  /*
   * Starting from version 0.11.0, wlroots rotates counter-clockwise, while
   * it was rotating clockwise previously.
   * In order to maintain the same behavior, we need to modify the transform
   * before applying it
   */
  switch (*transform) {
  case WL_OUTPUT_TRANSFORM_90:
    *transform = WL_OUTPUT_TRANSFORM_270;
    break;
  case WL_OUTPUT_TRANSFORM_270:
    *transform = WL_OUTPUT_TRANSFORM_90;
    break;
  case WL_OUTPUT_TRANSFORM_FLIPPED_90:
    *transform = WL_OUTPUT_TRANSFORM_FLIPPED_270;
    break;
  case WL_OUTPUT_TRANSFORM_FLIPPED_270:
    *transform = WL_OUTPUT_TRANSFORM_FLIPPED_90;
    break;
  default:
    /* Nothing to be done */
    break;
  }
}

#define MIN_WIDTH       360.0
#define MIN_HEIGHT      540.0
#define MAX_DPI_TARGET  180.0
#define INCH_IN_MM      25.4

/**
 * phoc_utils_compute_scale:
 * @phys_width: The physical width
 * @phys_height: The physical height
 * @width: The width in pixels
 * @height: The height in pixels
 *
 * Compute a suitable output scale based on the physical size and resolution.
 *
 * Returns: The output scale
 */
float
phoc_utils_compute_scale (int32_t phys_width, int32_t phys_height,
                          int32_t width, int32_t height)
{
  float dpi, max_scale, scale;

  // Ensure scaled resolution won't be inferior to minimum values
  max_scale = fminf (height / MIN_HEIGHT, width / MIN_WIDTH);

  /*
   * Round the maximum scale to a sensible value:
   *   - never use a scaling factor < 1
   *   - round to the lower 0.25 step below 2
   *   - round to the lower 0.5 step between 2 and 3
   *   - round to the lower integer value over 3
   */
  if (max_scale < 1) {
    max_scale = 1;
  } else if (max_scale < 2) {
    max_scale = 0.25 * floorf (max_scale / 0.25);
  } else if (max_scale < 3) {
    max_scale = 0.5 * floorf (max_scale / 0.5);
  } else {
    max_scale = floorf (max_scale);
  }

  dpi = (float) height / (float) phys_height * INCH_IN_MM;
  scale = fminf (ceilf (dpi / MAX_DPI_TARGET), max_scale);

  g_debug ("Output DPI is %f for mode %" PRId32 "x%" PRId32", using scale %f",
           dpi, width, height, scale);

  return scale;
}


static int
scale_length (int length, int offset, float scale)
{
  return round ((offset + length) * scale) - round (offset * scale);
}

/**
 * phoc_utils_scale_box:
 * @box: (inout): The box to scale
 * @scale: The scale to apply
 *
 * Scales the passed in box by scale.
 */
void
phoc_utils_scale_box (struct wlr_box *box, float scale)
{
  box->width = scale_length (box->width, box->x, scale);
  box->height = scale_length (box->height, box->y, scale);
  box->x = round (box->x * scale);
  box->y = round (box->y * scale);
}
