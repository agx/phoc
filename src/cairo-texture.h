/*
 * Copyright (C) 2025 The Phosh Developers
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define PHOC_TYPE_CAIRO_TEXTURE (phoc_cairo_texture_get_type ())

G_DECLARE_FINAL_TYPE (PhocCairoTexture, phoc_cairo_texture, PHOC, CAIRO_TEXTURE, GObject)

typedef struct _cairo cairo_t;

PhocCairoTexture   *phoc_cairo_texture_new         (int width, int height);
cairo_t            *phoc_cairo_texture_get_context (PhocCairoTexture *self);
struct wlr_texture *phoc_cairo_texture_get_texture (PhocCairoTexture *self);
void                phoc_cairo_texture_update      (PhocCairoTexture *self);

G_END_DECLS
